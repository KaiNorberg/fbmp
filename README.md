# Framebuffer BitMaP or .fbmp [![License](https://img.shields.io/badge/licence-MIT-green)](https://github.com/KaiNorberg/fbmp/blob/main/LICENSE)

The Framebuffer BitMaP format is designed to be as simple as possible, and to be directly compatible with GOP framebuffers.

## Why does this exist?

Imagine you wish to load and/or save an image from a program. You currently have two ways of accomplishing that, either use a third party library like [stb](https://github.com/nothings/stb) or create your own image loader, typically for .tga or .bmp files. Both of these approaches have rather significant drawbacks.

### Downsides of using third party libraries

If you choose to use a third party library you may introduce a significant amount of bloat to your program for something that should be as simple as loading an image, and it might just be impractical or impossible in an embedded environment.

### Downsides of making your own loader

If you choose to make your own image loader, then you run into the problem that making a fully featured parser for an image format as simple as .tga or .bmp still requires a significant amount of work. This usually results in programs with image loaders that are not fully featured, may not support all variants of a file format, or contain subtle bugs. For example .tga loaders may not support compression or different bits per pixel, which creates a frustrating situation of making sure that the image you are using is not just the right file format, but also that invisible "variables" within said file are correct for example bits per pixel.

### Minimal need for processing

The Framebuffer part of the name is important, since .fbmp uses a 8bpp ARGB pixel format, it is directly compatible with a lot of frame buffers, for example GOP frame buffers. Meaning there is no need for "parsing" or in any way processing the data contained within the file, it is intended to be used as is.

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

    // Do stuff with file

    printf("Magic: 0x%08x Width: 0x%08x Height: 0x%08x First Pixel: 0x%08x\n",
        image->magic, image->width, image->height, image->data[0]);

    free(image);

    return 0;
}
```
