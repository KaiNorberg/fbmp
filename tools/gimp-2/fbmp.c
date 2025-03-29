#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bzlib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <libgimp/gimp.h>

#define EXTENSIONS "fbmp"

#define PROC_LOAD "file_fbmp_load"
#define PROC_SAVE "file_fbmp_save"

static void query(void);
static void run(const gchar* name, gint nparams, const GimpParam* param, gint* nResults, GimpParam** results);

GimpPlugInInfo PLUG_IN_INFO = {
    NULL,  /* init_procedure */
    NULL,  /* quit_procedure */
    query, /* query_procedure */
    run,   /* run_procedure */
};

MAIN()

#define FBMP_MAGIC 0x706D6266

static void query(void)
{
    static const GimpParamDef loadArgs[] = {
        {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
        {GIMP_PDB_STRING, "filename", "The name of the file to load"},
        {GIMP_PDB_STRING, "raw_filename", "The name of the file to load"},
    };

    static const GimpParamDef saveArgs[] = {
        {GIMP_PDB_INT32, "run-mode", "Interactive, non-interactive"},
        {GIMP_PDB_IMAGE, "image", "Input image"},
        {GIMP_PDB_DRAWABLE, "drawable", "Drawable to save"},
        {GIMP_PDB_STRING, "filename", "The name of the file to save"},
        {GIMP_PDB_STRING, "raw-filename", "The name of the file to save"},
    };

    static const GimpParamDef loadresults[] = {
        {GIMP_PDB_IMAGE, "image", "Output image"},
    };

    gimp_install_procedure(PROC_LOAD, "Loads framebuffer bitmaps", "Loads framebuffer bitmaps.", "Kai Norberg",
        "Copyright Kai Norberg", "2024", "Framebuffer BitMaP", NULL, GIMP_PLUGIN, G_N_ELEMENTS(loadArgs),
        G_N_ELEMENTS(loadresults), loadArgs, loadresults);

    gimp_register_file_handler_mime(PROC_LOAD, "image/fbmp");
    gimp_register_magic_load_handler(PROC_LOAD, EXTENSIONS, "", "0,string,fbmp");

    gimp_install_procedure(PROC_SAVE, "Saves framebuffer bitmaps", "Saves framebuffer bitmaps.", "Kai Norberg",
        "Copyright Kai Norberg", "2024", "Framebuffer BitMaP", "RGB", GIMP_PLUGIN, G_N_ELEMENTS(saveArgs), 0, saveArgs, NULL);

    gimp_register_file_handler_mime(PROC_SAVE, "image/fbmp");
    gimp_register_save_handler(PROC_SAVE, EXTENSIONS, "");
}

static void run(const gchar* name, gint nparams, const GimpParam* param, gint* nResults, GimpParam** resultsIn)
{
    static GimpParam results[2];
    *resultsIn = results;

    if (strcmp(name, PROC_LOAD) == 0)
    {
        FILE* file = fopen(param[1].data.d_string, "rb");

        uint32_t header[3];
        fread(header, 1, sizeof(header), file);
        uint32_t magic = header[0];
        uint32_t width = header[1];
        uint32_t height = header[2];

        gint32 image = gimp_image_new(width, height, GIMP_RGB);
        gimp_image_set_filename(image, param[1].data.d_string);

        gint32 layer = gimp_layer_new(image, _("Background"), width, height, GIMP_RGBA_IMAGE, 100.0, GIMP_NORMAL_MODE);
        gimp_image_insert_layer(image, layer, 0, 0);

        GimpDrawable* drawable = gimp_drawable_get(layer);
        GimpPixelRgn pixelRegion;
        gimp_pixel_rgn_init(&pixelRegion, drawable, 0, 0, width, height, TRUE, FALSE);

        uint32_t* buffer = malloc(height * width * 4);

        for (size_t y = 0; y < height; y++)
        {
            for (size_t x = 0; x < width; x++)
            {
                uint32_t data;
                fread(&data, sizeof(uint32_t), 1, file);

                uint32_t pixel = htobe32((data << 8) | ((data >> 24) & 0xFF));
                buffer[x + y * width] = pixel;
            }
        }

        fclose(file);
        gimp_pixel_rgn_set_rect(&pixelRegion, (guchar*)buffer, 0, 0, width, height);
        free(buffer);
        gimp_drawable_flush(drawable);
        gimp_drawable_detach(drawable);

        *nResults = 2;
        results[0].type = GIMP_PDB_STATUS;
        results[0].data.d_status = GIMP_PDB_SUCCESS;
        results[1].type = GIMP_PDB_IMAGE;
        results[1].data.d_image = image;
    }
    else if (strcmp(name, PROC_SAVE) == 0)
    {
        GimpDrawable* drawable = gimp_drawable_get(param[2].data.d_int32);
        GimpImageType imageType = gimp_drawable_type(drawable->drawable_id);
        if (imageType != GIMP_RGBA_IMAGE)
        {
            *nResults = 2;
            results[0].type = GIMP_PDB_STATUS;
            results[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
            results[1].type = GIMP_PDB_STRING;
            results[1].data.d_string = "Image must be RGBA.";
            return;
        }

        GimpPixelRgn pixelRegion;
        gimp_pixel_rgn_init(&pixelRegion, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);

        FILE* file = fopen(param[3].data.d_string, "wb");

        uint32_t header[3];
        header[0] = FBMP_MAGIC;
        header[1] = drawable->width;
        header[2] = drawable->height;
        fwrite(&header, sizeof(header), 1, file);

        guchar* buffer = malloc(drawable->width * drawable->height * 4);
        gimp_pixel_rgn_get_rect(&pixelRegion, buffer, 0, 0, drawable->width, drawable->height);

        for (size_t y = 0; y < drawable->height; y++)
        {
            for (size_t x = 0; x < drawable->width; x++)
            {
                uint32_t data = ((uint32_t*)buffer)[x + y * drawable->width];

                uint32_t pixel = htobe32((data << 8) | ((data >> 24) & 0xFF));
                fwrite(&pixel, sizeof(uint32_t), 1, file);
            }
        }

        free(buffer);
        fclose(file);
        gimp_drawable_detach(drawable);

        *nResults = 1;
        results[0].type = GIMP_PDB_STATUS;
        results[0].data.d_status = GIMP_PDB_SUCCESS;
    }
    else
    {
        *nResults = 1;
        results[0].type = GIMP_PDB_STATUS;
        results[0].data.d_status = GIMP_PDB_CALLING_ERROR;
    }
}
