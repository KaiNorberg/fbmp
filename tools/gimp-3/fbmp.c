#include <libgimp/gimp.h>
#include <stdio.h>

#define PROC_LOAD "plug-in-fbmp-load"
#define PROC_EXPORT "plug-in-fbmp-export"

#define FBMP_MAGIC 0x706D6266

struct _Fbmp
{
    GimpPlugIn parent_instance;
};

#define FBMP_TYPE (fbmp_get_type())
G_DECLARE_FINAL_TYPE(Fbmp, fbmp, HELLO_WORLD, , GimpPlugIn)

static GList* fbmp_query_procedures(GimpPlugIn* plug_in);
static GimpProcedure* fbmp_create_procedure(GimpPlugIn* plug_in, const gchar* name);

G_DEFINE_TYPE(Fbmp, fbmp, GIMP_TYPE_PLUG_IN)

static void fbmp_class_init(FbmpClass* klass)
{
    GimpPlugInClass* plug_in_class = GIMP_PLUG_IN_CLASS(klass);

    plug_in_class->query_procedures = fbmp_query_procedures;
    plug_in_class->create_procedure = fbmp_create_procedure;
}

static void fbmp_init(Fbmp* fbmp)
{
}

static GList* fbmp_query_procedures(GimpPlugIn* plug_in)
{
    GList* list = g_list_append(NULL, g_strdup(PROC_LOAD));
    list = g_list_append(list, g_strdup(PROC_EXPORT));

    return list;
}

typedef struct fbmp
{
    uint32_t magic;
    uint32_t width;
    uint32_t height;
    uint32_t data[];
} fbmp_t;

static GimpValueArray* fbmp_load(GimpProcedure* procedure, GimpRunMode run_mode, GFile* file, GimpMetadata* metadata,
    GimpMetadataLoadFlags* flags, GimpProcedureConfig* config, gpointer run_data)
{
    GBytes* bytes = g_file_load_bytes(file, NULL, NULL, NULL);
    const fbmp_t* header = (const fbmp_t*)g_bytes_get_data(bytes, NULL);

    if (header->magic != FBMP_MAGIC)
    {
        g_bytes_unref(bytes);
        return gimp_procedure_new_return_values(procedure, GIMP_PDB_EXECUTION_ERROR, NULL);
    }

    GimpImage* image = gimp_image_new(header->width, header->height, GIMP_RGB);
    GimpLayer* layer =
        gimp_layer_new(image, "Background", header->width, header->height, GIMP_RGBA_IMAGE, 100.0, GIMP_LAYER_MODE_NORMAL);
    gimp_image_insert_layer(image, layer, NULL, 0);

    GeglBuffer* buffer = gimp_drawable_get_buffer(GIMP_DRAWABLE(layer));

    const guint8* pixel_data = (const guint8*)(header->data);
    GeglRectangle rect = {0, 0, header->width, header->height};

    gegl_buffer_set(buffer, &rect, 0, NULL, pixel_data, GEGL_AUTO_ROWSTRIDE);

    GimpValueArray* result = gimp_procedure_new_return_values(procedure, GIMP_PDB_SUCCESS, NULL);
    GValue* value = gimp_value_array_index(result, 1);
    g_value_init(value, GIMP_TYPE_IMAGE);
    g_value_set_object(value, image);

    g_bytes_unref(bytes);
    g_object_unref(buffer);

    return result;
}

static GimpValueArray* fbmp_export(GimpProcedure* procedure, GimpRunMode run_mode, GimpImage* image, GFile* file,
    GimpExportOptions* options, GimpMetadata* metadata, GimpProcedureConfig* config, gpointer run_data)
{
    GimpLayer** layers = gimp_image_get_layers(image);
    if (!layers)
    {
        return gimp_procedure_new_return_values(procedure, GIMP_PDB_EXECUTION_ERROR, NULL);
    }
    GimpLayer* layer = layers[0];

    gint width = gimp_drawable_get_width(GIMP_DRAWABLE(layer));
    gint height = gimp_drawable_get_height(GIMP_DRAWABLE(layer));

    GeglBuffer* buffer = gimp_drawable_get_buffer(GIMP_DRAWABLE(layer));
    gsize dataSize = width * height * 4;
    guchar* pixels = g_malloc(dataSize);

    GeglRectangle rect = {0, 0, width, height};
    gegl_buffer_get(buffer, &rect, 1.0, NULL, pixels, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

    fbmp_t* fbmp = g_malloc(sizeof(uint32_t) * 3 + dataSize);

    fbmp->magic = FBMP_MAGIC;
    fbmp->width = width;
    fbmp->height = height;

    memcpy(fbmp->data, pixels, dataSize);

    GBytes* bytes = g_bytes_new(fbmp, sizeof(uint32_t) * 3 + dataSize);
    GError* error = NULL;
    gboolean success = g_file_replace_contents(file, g_bytes_get_data(bytes, NULL), g_bytes_get_size(bytes), NULL, FALSE,
        G_FILE_CREATE_NONE, NULL, NULL, &error);

    g_free(pixels);
    g_free(fbmp);
    g_free(layers);
    g_bytes_unref(bytes);
    g_object_unref(buffer);

    if (!success && error)
    {
        gimp_message(error->message);
        g_error_free(error);
        return gimp_procedure_new_return_values(procedure, GIMP_PDB_EXECUTION_ERROR, NULL);
    }

    return gimp_procedure_new_return_values(procedure, GIMP_PDB_SUCCESS, NULL);
}

static GimpProcedure* fbmp_create_procedure(GimpPlugIn* plug_in, const gchar* name)
{
    GimpProcedure* procedure = NULL;
    if (g_strcmp0(name, PROC_LOAD) == 0)
    {
        procedure = gimp_load_procedure_new(plug_in, name, GIMP_PDB_PROC_TYPE_PLUGIN, fbmp_load, NULL, NULL);
        gimp_procedure_set_image_types(procedure, NULL); // Accepts any image type
                
        gimp_procedure_add_image_return_value(procedure, NULL, NULL, NULL, 0, 0);
    }
    else if (g_strcmp0(name, PROC_EXPORT) == 0)
    {
        procedure = gimp_export_procedure_new(plug_in, name, GIMP_PDB_PROC_TYPE_PLUGIN, FALSE, fbmp_export, NULL, NULL);
    }

    GimpFileProcedure* fileProc = GIMP_FILE_PROCEDURE(procedure);

    gimp_procedure_set_sensitivity_mask(procedure, GIMP_PROCEDURE_SENSITIVE_ALWAYS);
    gimp_procedure_set_menu_label(procedure, "Framebuffer BitMaP");
    gimp_procedure_set_documentation(procedure, "Plugin for Framebuffer BitMaP files", NULL, NULL);
    gimp_procedure_set_attribution(procedure, "Kai Norberg", "Kai Norberg", "2025");

    gimp_file_procedure_set_priority(fileProc, 0);      
    gimp_file_procedure_set_magics(fileProc, "0,string,fbmp");
    gimp_file_procedure_set_mime_types(fileProc, "image/fbmp");

    if (g_strcmp0(name, PROC_LOAD) == 0)
    {
        // For some reason adding file extensions to the export proc causes gimp to think the export proc is a load proc???
        gimp_procedure_add_menu_path(procedure, "<Image>/File/Open");
        gimp_file_procedure_set_extensions(fileProc, "fbmp");
    }
    else if (g_strcmp0(name, PROC_EXPORT) == 0)
    {
        gimp_procedure_add_menu_path(procedure, "<Image>/File/Export");
    }

    return procedure;
}

GIMP_MAIN(FBMP_TYPE)