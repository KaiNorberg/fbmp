# Framebuffer BitMaP or .fbmp [![License: Unlicense](https://img.shields.io/badge/license-Unlicense-blue.svg)](http://unlicense.org/)

The Framebuffer BitMaP format is designed to be as simple as possible, and to be directly compatible with GOP framebuffers. The point is not to be revolutionary but to provide a simple standard and a list of tools for the standard.

## Why does this exist?

Imagine you wish to load and/or save an image from a program. You currently have two ways of accomplishing that, either use a third party library like [stb](https://github.com/nothings/stb) or create your own image loader, typically for .tga or .bmp files. Both of these approaches have rather significant drawbacks.

### Downsides of using third party libraries

If you choose to use a third party library you may introduce a significant amount of bloat to your program for something that should be as simple as loading an image, and it might just be impractical or impossible in an embedded environment.

### Downsides of making your own loader

If you choose to make your own image loader, then you run into the problem that making a fully featured parser even for an image format as simple as .tga or .bmp still requires a significant amount of work. This usually results in programs with image loaders that are not fully featured, may not support all variants of a file format, or contain subtle bugs. For example custom made .tga loaders may not support compression or different bits per pixel, which creates a frustrating situation of making sure that the image you are using is not just the right file format, but also that invisible "variables" within said file are correct for example bits per pixel.

### Minimal need for processing

The Framebuffer part of the name is important, since .fbmp uses a 32 bit ARGB pixel format, it is directly compatible with a lot of frame buffers, for example GOP frame buffers. Meaning there is no need for "parsing" or in any way processing the data contained within the file, it is intended to be used as is, which makes it very fast and efficient.

### Image compression

The .fbmp file format does not directly support compression, but it is also not needed, as testing has shown that compressing the files via tools like zip or gz results in file sizes that are the same if not smaller to that of a .png equivalent. Thus, if you really need compression, it's often better to simply compress a .fbmp file then use a "natively" compressed image file. Following is a comparison of the following 1406x1384 image in different formats:

<div align="center">

<img src="https://github.com/KaiNorberg/fbmp/assets/73393684/cccda9b6-129e-4bc6-bbac-c66b9da3a3e0" width="250">

| Size | Format |
| -------- | ------- |
| 19.3 MB | .fbmp | 
| 4.4 MB | .png | 
| 4.4 MB | .png.tar.gz | 
| 1.7 MB | .fbmp.tar.gz | 
| 855 KB | .jpg | 
| 845 KB | .jpg.tar.gz |
    
</div>

## Tools

Below is a list of the currently available tools for .fbmp.

### Gimp plugin

Allows for loading and saving of .fbmp files via gimp.

The plugin can be installed with the following steps.

1. Clone (download) this repository, you can use the Code button at the top left of the screen, or if you have git installed use the following command `git clone --recursive https://github.com/KaiNorberg/fbmp`.
2. Figure out which version of gimp you have, if your not sure you most likely have gimp-3.
3. Move into the fbmp/tools/gimp-* directory with the following command `cd fbmp/tools/gimp-*`
4. Run the `make all install` command to both build and install the plugin.

* The version of gimp you have, either 2 or 3.

Done! 

If you wish to uninstall the plugin just move back to the fbmp/tools/gimp directory and run `make uninstall`.

## Implementations

* [PatchworkOS](https://github.com/KaiNorberg/PatchworkOS) hobbyist operating system
* [mango](https://github.com/t0rakka/mango) image loader (for... reasons they call it .fpng not .fbmp)

## Format

The image data is stored in BGRA byte order. When read as a 32-bit integer, because the format uses little-endian, it will appear as ARGB.

The following describes the format of a .fbmp file, starting from the beginning of the file. An example image can also be found in the example folder.

<div align="center">

| Offset | Size | Description |
| -------- | ------- | -------  |
| 0 | 4 | Magic number = 0x706D6266 |
| 4 | 4 | Unsigned width in pixels |
| 8 | 4 | Unsigned height in pixels |
| 12 | 4 * width * height | Image data, row-major |

\* Offset and Size is provided in bytes.
</div>
 
## Example image loader in C

The following is a fully featured .fbmp loader in C:

```c
typedef struct fbmp
{
    uint32_t magic;
    uint32_t width;
    uint32_t height;
    uint32_t data[];
} fbmp_t;

int main()
{
    FILE* file = fopen("path/to/image.fbmp", "rb");

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    fbmp_t* image = malloc(fileSize);
    fread(image, fileSize, 1, file);

    fclose(file);

    if (image->magic != 0x706D6266)
    {
        // Handle corrupt file
    }

    // Do stuff with the image

    printf("Magic: 0x%08x Width: 0x%08x Height: 0x%08x First Pixel: 0x%08x\n",
        image->magic, image->width, image->height, image->data[0]);

    free(image);

    return 0;
}
```

## Contributing

If you end up creating more tools/plugins or anything else related to the .fbmp file format feel free to submit a pull request to have it added to this repository.
