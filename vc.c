//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT�CNICO DO C�VADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORM�TICOS
//                    VIS�O POR COMPUTADOR
//
//             [  BRUNO OLIVEIRA - boliveira@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Desabilita (no MSVC++) warnings de fun��es n�o seguras (fopen, sscanf, etc...)
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include "vc.h"

#define MAX3(r, g, b) (r > g ? (r > b ? r : b) : (g > b ? g : b)) 
#define MIN3(r, g, b) (r < g ? (r < b ? r : b) : (g < b ? g : b)) 

#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/*
int main(void)
{
    IVC *image, *image2, *image3;
    int i;
    char imagem_original[] = "Images/labelling.pgm";
    image = vc_read_image(imagem_original);
    if (image == NULL)
    {
        printf("Error");
        getchar();
        return 0;
    }
    IVC *new_image = vc_image_new(image->width, image->height, 1, 1);
    IVC *new_image2 = vc_image_new(image->width, image->height, 1, 255);
    vc_gray_to_binary(image,new_image,127);
    vc_binary_blob_labelling(new_image,new_image2);
    vc_write_image("new_image.pgm", new_image2);

    vc_image_free(image);
    printf("press any key to exit");
    char program[] = "gimp ";

    system(strcat(program, imagem_original));
    system("gimp new_image.pgm &");

    getchar();
    return 0;
}*/

// Alocar mem�ria para uma imagem
IVC* vc_image_new(int width, int height, int channels, int levels)
{
    IVC* image = (IVC*)malloc(sizeof(IVC));

    if (image == NULL)
        return NULL;
    if ((levels <= 0) || (levels > 255))
        return NULL;

    image->width = width;
    image->height = height;
    image->channels = channels;
    image->levels = levels;
    image->bytesperline = image->width * image->channels;
    image->data = (unsigned char*)malloc(image->width * image->height * image->channels * sizeof(char));

    if (image->data == NULL)
    {
        return vc_image_free(image);
    }

    return image;
}

// Libertar mem�ria de uma imagem
IVC* vc_image_free(IVC* image)
{
    if (image != NULL)
    {
        if (image->data != NULL)
        {
            free(image->data);
            image->data = NULL;
        }

        free(image);
        image = NULL;
    }

    return image;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUN��ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

char* netpbm_get_token(FILE* file, char* tok, int len)
{
    char* t;
    int c;

    for (;;)
    {
        while (isspace(c = getc(file)))
            ;
        if (c != '#')
            break;
        do
            c = getc(file);
        while ((c != '\n') && (c != EOF));
        if (c == EOF)
            break;
    }

    t = tok;

    if (c != EOF)
    {
        do
        {
            *t++ = c;
            c = getc(file);
        } while ((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));

        if (c == '#')
            ungetc(c, file);
    }

    *t = 0;

    return tok;
}

long int unsigned_char_to_bit(unsigned char* datauchar, unsigned char* databit, int width, int height)
{
    int x, y;
    int countbits;
    long int pos, counttotalbytes;
    unsigned char* p = databit;

    *p = 0;
    countbits = 1;
    counttotalbytes = 0;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos = width * y + x;

            if (countbits <= 8)
            {
                // Numa imagem PBM:
                // 1 = Preto
                // 0 = Branco
                //*p |= (datauchar[pos] != 0) << (8 - countbits);

                // Na nossa imagem:
                // 1 = Branco
                // 0 = Preto
                *p |= (datauchar[pos] == 0) << (8 - countbits);

                countbits++;
            }
            if ((countbits > 8) || (x == width - 1))
            {
                p++;
                *p = 0;
                countbits = 1;
                counttotalbytes++;
            }
        }
    }

    return counttotalbytes;
}

void bit_to_unsigned_char(unsigned char* databit, unsigned char* datauchar, int width, int height)
{
    int x, y;
    int countbits;
    long int pos;
    unsigned char* p = databit;

    countbits = 1;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos = width * y + x;

            if (countbits <= 8)
            {
                // Numa imagem PBM:
                // 1 = Preto
                // 0 = Branco
                // datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

                // Na nossa imagem:
                // 1 = Branco
                // 0 = Preto
                datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;

                countbits++;
            }
            if ((countbits > 8) || (x == width - 1))
            {
                p++;
                countbits = 1;
            }
        }
    }
}

IVC* vc_read_image(char* filename)
{
    FILE* file = NULL;
    IVC* image = NULL;
    unsigned char* tmp;
    char tok[20];
    long int size, sizeofbinarydata;
    int width, height, channels;
    int levels = 255;
    int v;

    // Abre o ficheiro
    if ((file = fopen(filename, "rb")) != NULL)
    {
        // Efectua a leitura do header
        netpbm_get_token(file, tok, sizeof(tok));

        if (strcmp(tok, "P4") == 0)
        {
            channels = 1;
            levels = 1;
        } // Se PBM (Binary [0,1])
        else if (strcmp(tok, "P5") == 0)
            channels = 1; // Se PGM (Gray [0,MAX(level,255)])
        else if (strcmp(tok, "P6") == 0)
            channels = 3; // Se PPM (RGB [0,MAX(level,255)])
        else
        {
#ifdef VC_DEBUG
            printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
#endif

            fclose(file);
            return NULL;
        }

        if (levels == 1) // PBM
        {
            if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
                sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
            {
#ifdef VC_DEBUG
                printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
#endif

                fclose(file);
                return NULL;
            }

            // Aloca mem�ria para imagem
            image = vc_image_new(width, height, channels, levels);
            if (image == NULL)
                return NULL;

            sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
            tmp = (unsigned char*)malloc(sizeofbinarydata);
            if (tmp == NULL)
                return 0;

#ifdef VC_DEBUG
            printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

            if ((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
            {
#ifdef VC_DEBUG
                printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

                vc_image_free(image);
                fclose(file);
                free(tmp);
                return NULL;
            }

            bit_to_unsigned_char(tmp, image->data, image->width, image->height);

            free(tmp);
        }
        else // PGM ou PPM
        {
            if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
                sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 ||
                sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
            {
#ifdef VC_DEBUG
                printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
#endif

                fclose(file);
                return NULL;
            }

            // Aloca mem�ria para imagem
            image = vc_image_new(width, height, channels, levels);
            if (image == NULL)
                return NULL;

#ifdef VC_DEBUG
            printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

            size = image->width * image->height * image->channels;

            if ((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
            {
#ifdef VC_DEBUG
                printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

                vc_image_free(image);
                fclose(file);
                return NULL;
            }
        }

        fclose(file);
    }
    else
    {
#ifdef VC_DEBUG
        printf("ERROR -> vc_read_image():\n\tFile not found.\n");
#endif
    }

    return image;
}

int vc_write_image(char* filename, IVC* image)
{
    FILE* file = NULL;
    unsigned char* tmp;
    long int totalbytes, sizeofbinarydata;

    if (image == NULL)
        return 0;

    if ((file = fopen(filename, "wb")) != NULL)
    {
        if (image->levels == 1)
        {
            sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
            tmp = (unsigned char*)malloc(sizeofbinarydata);
            if (tmp == NULL)
                return 0;

            fprintf(file, "%s %d %d\n", "P4", image->width, image->height);

            totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
            printf("Total = %ld\n", totalbytes);
            if (fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
            {
#ifdef VC_DEBUG
                fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

                fclose(file);
                free(tmp);
                return 0;
            }

            free(tmp);
        }
        else
        {
            fprintf(file, "%s %d %d 255\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height);

            if (fwrite(image->data, image->bytesperline, image->height, file) != image->height)
            {
#ifdef VC_DEBUG
                fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

                fclose(file);
                return 0;
            }
        }

        fclose(file);

        return 1;
    }

    return 0;
}

int vc_gray_negative(IVC* srcdst)
{
    unsigned char* data = (unsigned char*)srcdst->data;
    int width = srcdst->width;
    int height = srcdst->height;
    int bytesperline = srcdst->width * srcdst->channels;
    int channels = srcdst->channels;
    int x, y;
    long int pos;

    // Verifica��o de erros
    if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))
        return 0;
    if (channels != 1)
        return 0;

    // Inverte a imagem Gray
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos = y * bytesperline + x * channels;

            data[pos] = 255 - data[pos];
        }
    }

    return 1;
}

int vc_rgb_negative(IVC* srcdst)
{
    unsigned char* data = (unsigned char*)srcdst->data;
    int width = srcdst->width;
    int height = srcdst->height;
    int bytesperline = srcdst->width * srcdst->channels;
    int channels = srcdst->channels;
    int x, y;
    long int pos;

    // Verifica��o de erros
    if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))
        return 0;
    if (channels != 3)
        return 0;

    // Inverte a imagem Gray
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos = y * bytesperline + x * channels;

            data[pos] = 255 - data[pos];
            data[pos + 1] = 255 - data[pos + 1];
            data[pos + 2] = 255 - data[pos + 2];
        }
    }

    return 1;
}

int vc_rgb_get_red_gray(IVC* srcdst)
{
    unsigned char* data = (unsigned char*)srcdst->data;
    int width = srcdst->width;
    int height = srcdst->height;
    int bytesperline = srcdst->width * srcdst->channels;
    int channels = srcdst->channels;
    int x, y;
    long int pos;

    // Verifica��o de erros
    if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))
        return 0;
    if (channels != 3)
        return 0;

    // Inverte a imagem Gray
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos = y * bytesperline + x * channels;

            data[pos + 1] = data[pos];
            data[pos + 2] = data[pos];
        }
    }

    return 1;
}

int vc_rgb_get_green_gray(IVC* srcdst)
{
    unsigned char* data = (unsigned char*)srcdst->data;
    int width = srcdst->width;
    int height = srcdst->height;
    int bytesperline = srcdst->width * srcdst->channels;
    int channels = srcdst->channels;
    int x, y;
    long int pos;

    // Verifica��o de erros
    if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))
        return 0;
    if (channels != 3)
        return 0;

    // Inverte a imagem Gray
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos = y * bytesperline + x * channels;

            data[pos] = data[pos + 1];
            data[pos + 2] = data[pos + 1];
        }
    }
    return 1;
}

int vc_rgb_get_blue_gray(IVC* srcdst)
{
    unsigned char* data = (unsigned char*)srcdst->data;
    int width = srcdst->width;
    int height = srcdst->height;
    int bytesperline = srcdst->width * srcdst->channels;
    int channels = srcdst->channels;
    int x, y;
    long int pos;

    // Verifica��o de erros
    if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))
        return 0;
    if (channels != 3)
        return 0;

    // Inverte a imagem Gray
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {

            pos = y * bytesperline + x * channels;
            data[pos] = data[pos + 2];
            data[pos + 1] = data[pos + 2];
        }
    }

    return 1;
}

int vc_rgb_to_gray(IVC* src, IVC* dst)
{
    unsigned char* datasrc = (unsigned char*)src->data;
    int bytesperline_src = src->width * src->channels;
    int channels_src = src->channels;
    unsigned char* datadst = (unsigned char*)dst->data;
    int bytesperline_dst = dst->width * dst->channels;
    int channels_dst = dst->channels;
    int width = src->width;
    int height = src->height;
    int x, y;
    long int pos_src, pos_dst;
    float rf, gf, bf;

    // Verifica��o de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
        return 0;

    if ((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL))
        return 0;

    if ((src->channels != 3) || (dst->channels != 1))
        return 0;

    //
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos_src = (y * bytesperline_src) + (x * channels_src);
            pos_dst = (y * bytesperline_dst) + (x * channels_dst);

            rf = (float)datasrc[pos_src];
            gf = (float)datasrc[pos_src + 1];
            bf = (float)datasrc[pos_src + 2];

            datadst[pos_dst] = (unsigned char)((rf * 0.299) + (gf * 0.587) + (bf * 0.114));
        }
    }

    return 1;
}

int vc_rgb_to_hsv(IVC  *src, IVC *dst) 
{
    if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels > 255))
        return 0;
    if (src->channels != 3 || dst->channels != 3)
        return 0;

    int x, y; 
    float matiz, sat, valor, max, min, r, g, b;
    long int possrc, posdst;

    for (x = 0; x < src->width; x++)
    {
        for (y = 0; y < src->height; y++)
        {
            possrc = y * src->bytesperline + x * src->channels;
            posdst = y * dst->bytesperline + x * dst->channels;

            max = min = src->data[possrc];

            r = src->data[possrc];
            g = src->data[possrc + 1];
            b = src->data[possrc + 2];
            max = MAX3(r, g, b);
            min = MIN3(r, g, b);

            valor = max;
            if (valor > 0 && (max - min) > 0)
                sat = 255.0f * (max - min) / valor;
            else
                sat = 0;

            if (sat == 0)
                matiz = 0;
            else
            {
                if ((max == src->data[possrc]) && (src->data[possrc + 1] > src->data[possrc + 2]))
                    matiz = 60 * (src->data[possrc + 1] - src->data[possrc + 2]) / (max - min);
                else if (max == src->data[possrc] && src->data[possrc + 2] > src->data[possrc + 1])
                    matiz = 360 + 60 * (src->data[possrc + 1] - src->data[possrc + 2]) / (max - min);
                else if (max == src->data[possrc + 1])
                    matiz = 120 + 60 * (src->data[possrc + 2] - src->data[possrc]) / (max - min);
                else if (max == src->data[possrc + 2])
                    matiz = 240 + 60 * (src->data[possrc] - src->data[possrc + 1]) / (max - min);
                else
                    matiz = 0;
            }
            dst->data[posdst] = (int)(matiz * 255.0f) / 360;
            dst->data[posdst + 1] = (int)sat;
            dst->data[posdst + 2] = (int)valor;
        }
    }
    return 0;
}

IVC* vc_convert_bgr_to_rgb(IVC* src)
{
    unsigned char* datadst = src->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;

    //Verificacao de erros
    if ((width <= 0) || (height <= 0) || (datadst == NULL)) return 0;
    if (channels != 3) return 0;
    //Verifica se existe blobs

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int pos = y * bytesperline + x * channels;
            unsigned char aux = datadst[pos];
            datadst[pos] = datadst[pos + 2];
            datadst[pos + 2] = aux;
        }
    }
    return src;
}

int vc_rgb_to_hsv2(IVC* srcdst)
{
    unsigned char* data = (unsigned char*)srcdst->data;
    int width = srcdst->width;
    int height = srcdst->height;
    int bytesperline = srcdst->bytesperline;
    int channels = srcdst->channels;
    float r, g, b, hue, saturation, value;
    float rgb_max, rgb_min;
    int i, size;

    // Verificação de erros
    if ((width <= 0) || (height <= 0) || (data == NULL)) return 0;
    if (channels != 3) return 0;

    size = width * height * channels;

    for (i = 0; i < size; i = i + channels)
    {
        r = (float)data[i];
        g = (float)data[i + 1];
        b = (float)data[i + 2];

        // Calcula valores máximo e mínimo dos canais de cor R, G e B
        rgb_max = (r > g ? (r > b ? r : b) : (g > b ? g : b));
        rgb_min = (r < g ? (r < b ? r : b) : (g < b ? g : b));

        // Value toma valores entre [0,255]
        value = rgb_max;
        if (value == 0.0f)
        {
            hue = 0.0f;
            saturation = 0.0f;
        }
        else
        {
            // Saturation toma valores entre [0,255]
            saturation = ((rgb_max - rgb_min) / rgb_max) * 255.0f;

            if (saturation == 0.0f)
            {
                hue = 0.0f;
            }
            else
            {
                // Hue toma valores entre [0,360]
                if ((rgb_max == r) && (g >= b))
                {
                    hue = 60.0f * (g - b) / (rgb_max - rgb_min);
                }
                else if ((rgb_max == r) && (b > g))
                {
                    hue = 360.0f + 60.0f * (g - b) / (rgb_max - rgb_min);
                }
                else if (rgb_max == g)
                {
                    hue = 120.0f + 60.0f * (b - r) / (rgb_max - rgb_min);
                }
                else
                {
                    hue = 240.0f + 60.0f * (r - g) / (rgb_max - rgb_min);
                }
            }
        }

        // Atribui valores entre [0,255]
        data[i] = (unsigned char)(hue / 360.0f * 255.0f);
        data[i + 1] = (unsigned char)(saturation);
        data[i + 2] = (unsigned char)(value);

    }

    return 1;
}

int getMax(int r, int g, int b)
{

    int valor = 0;

    if (valor < r)
        valor = r;

    if (valor < g)
        valor = g;

    if (valor < b)
        valor = b;

    return valor;
}

int getMin(int r, int g, int b)
{

    int valor = 255;

    if (valor > r)
        valor = r;

    if (valor > g)
        valor = g;

    if (valor > b)
        valor = b;

    return valor;
}

int vc_hsv_segmentation(IVC* src, IVC* dst)
{
    unsigned char* datasrc = (unsigned char*)src->data;
    int byterperline_src = src->width * src->channels;
    int channels_src = src->channels;
    unsigned char* datadst = (unsigned char*)dst->data;
    int bytesperline_dst = dst->width * dst->channels;
    int channels_dst = dst->channels;
    int width = src->width;
    int height = src->height;
    int x, y;
    long int pos_src, pos_dst;
    float h, s, v;

    if (src->width <= 0 || src->height <= 0 || src->data == NULL)
        return 0;
    if (src->width != dst->width || src->height != dst->height)
        return 0;
    if (src->channels != 3 || dst->channels != 1)
        return 0;

    // Segmentation loop
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos_src = y * byterperline_src + x * channels_src;
            pos_dst = y * bytesperline_dst + x * channels_dst;

            // Extract HSV values directly from the src image
            h = ((float)datasrc[pos_src]) / 255.0f * 360.0f;
            s = ((float)datasrc[pos_src + 1]) / 255.0f * 100.0f;
            v = ((float)datasrc[pos_src + 2]) / 255.0f * 100.0f;


            /* ISTO ESTAVA DENTRO DO IF EM BAIXO PARA FAZER SEGMENTAÇÃO DAS CORES (VERDE-AZUL-VERMELHO)
            * ||
                (h >= 67 && h <= 110 && s >= 25 && s <= 50 && v >= 37 && v <= 50) ||
                (h >= 115 && h <= 200 && s >= 10 && s <= 43 && v >= 35 && v <= 48) ||
                (h >= 30 && h <= 100 && s >= 3 && s <= 35 && v >= 22 && v <= 27) 
            * 
            */


            // Check if the pixel falls within the specified HSV range (resistor || green || blue || red)
            if ( ( ( h >= 30 && h <= 45 ) && ( s >= 45 && s <= 65 ) && ( v >= 50 && v <= 90 ) ) )
            {
                datadst[pos_dst] = 255; // Pixel is within range, mark as white 
            }
            else
            {
                datadst[pos_dst] = 0; // Pixel is outside range, mark as black 
            }
        }
    }

    return 1; // Success
}

int vc_gray_3channels(IVC* src, IVC* dst)
{
    unsigned char* datasrc = (unsigned char*)src->data;
    int bytesperline_src = src->width * src->channels;
    int channels_src = src->channels;
    unsigned char* datadst = (unsigned char*)dst->data;
    int bytesperline_dst = dst->width * dst->channels;
    int channels_dst = dst->channels;
    int width = src->width;
    int height = src->height;
    int x, y;
    long int pos_src, pos_dst;

    // Verificacao de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height)) return 0;
    if ((src->channels != 1) || (dst->channels != 3)) return 0;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos_src = y * bytesperline_src + x * channels_src; //Calcular indíce, channels_src = 1
            pos_dst = y * bytesperline_dst + x * channels_dst; // channels_dst = 3

            datadst[pos_dst] = datasrc[pos_src];
            datadst[pos_dst + 1] = datasrc[pos_src];
            datadst[pos_dst + 2] = datasrc[pos_src];
        }
    }
    return 1;
}

int vc_scale_gray_to_rgb(IVC* src, IVC* dst)
{
    unsigned char* datasrc = (unsigned char*)src->data;
    int bytesperline_src = src->width * src->channels;
    int channels_src = src->channels;
    unsigned char* datadst = (unsigned char*)dst->data;
    int bytesperline_dst = dst->width * dst->channels;
    int channels_dst = dst->channels;
    int width = src->width;
    int height = src->height;
    int x, y;
    long int pos_src, pos_dst;
    float rf, gf, bf;
    float gray;

    // Verifica��o de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
        return 0;

    if ((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL))
        return 0;

    if ((src->channels != 1) || (dst->channels != 3))
        return 0;

    //
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos_src = (y * bytesperline_src) + (x * channels_src);
            pos_dst = (y * bytesperline_dst) + (x * channels_dst);

            gray = (float)datasrc[pos_src];

            if (gray >= 0 && gray <= 64)
            {
                rf = 0;
                gf = gray * 4;
                bf = 255;
            }
            else if (gray > 64 && gray <= 128)
            {
                rf = 0;
                gf = 255;
                bf = 255 - ((gray - 64) * 4);
            }
            else if (gray > 128 && gray <= 192)
            {
                rf = (gray - 128) * 4;
                gf = 255;
                bf = 0;
            }
            else
            {
                rf = 255;
                gf = 255 - ((gray - 192) * 4);
                bf = 0;
            }

            datadst[pos_dst] = rf;
            datadst[pos_dst + 1] = gf;
            datadst[pos_dst + 2] = bf;
        }
    }

    return 1;
}

int vc_gray_to_binary(IVC* src, IVC* dst, int threshold)
{
    unsigned char* datasrc = (unsigned char*)src->data;
    int bytesperline_src = src->width * src->channels;
    int channels_src = src->channels;
    unsigned char* datadst = (unsigned char*)dst->data;
    int bytesperline_dst = dst->width * dst->channels;
    int channels_dst = dst->channels;
    int width = src->width;
    int height = src->height;
    int x, y;
    long int pos_src, pos_dst;
    float rf, gf, bf, pixel;

    // Verifica��o de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
        return 0;

    if ((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL))
        return 0;

    if ((src->channels != 1) || (dst->channels != 1))
        return 0;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos_src = (y * bytesperline_src) + (x * channels_src);
            pos_dst = (y * bytesperline_dst) + (x * channels_dst);
            pixel = (float)datasrc[pos_src];

            if (pixel >= threshold)
            {
                datadst[pos_dst] = 1;
                continue;
            }
            datadst[pos_dst] = 0;
        }
    }
    return 1;
}

int vc_gray_to_binary_midpoint(IVC* src, IVC* dst, int kernel_size)
{
    unsigned char* datasrc = (unsigned char*)src->data;
    int bytesperline_src = src->width * src->channels;
    int channels_src = src->channels;
    unsigned char* datadst = (unsigned char*)dst->data;
    int bytesperline_dst = dst->width * dst->channels;
    int channels_dst = dst->channels;
    int width = src->width;
    int height = src->height;
    int x, y;
    long int pos_src, pos_dst;
    float rf, gf, bf, pixel;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos_src = (y * bytesperline_src) + (x * channels_src);
            pos_dst = (y * bytesperline_dst) + (x * channels_dst);
            pixel = (float)datasrc[pos_src];
            int vmin = 255;
            int vmax = 0;
            for (int ky = y - (kernel_size / 2); ky < y + (kernel_size / 2); ++ky)
            {
                for (int kx = x - (kernel_size / 2); kx < x + (kernel_size / 2); ++kx)
                {
                    if (ky < 0 || ky >= height || kx < 0 || kx >= width)
                    {
                        continue;
                    }
                    int kpos = (ky * bytesperline_dst) + (kx * channels_dst);
                    int kpixel = (int)datasrc[kpos];
                    if (kpixel < vmin)
                    {
                        vmin = kpixel;
                    }
                    else if (kpixel > vmax)
                    {
                        vmax = kpixel;
                    }
                }
            }
            int t = (vmax + vmin) / 2;
            if (t < (int)pixel)
            {
                datadst[pos_dst] = 1;
            }
            else
            {
                datadst[pos_dst] = 0;
            }
        }
    }
}

int vc_binary_dilate(IVC* src, IVC* dst, int kernel)
{

    if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
        return 0;
    if (src->channels != 1 || dst->channels != 1)
        return 0;

    int x, y, tempx, tempy, posx, posy, teste = 0;
    long int possrc, posdst, pos;

    for (x = 0; x < src->width; x++)
    {
        for (y = 0; y < src->height; y++)
        {
            possrc = y * src->bytesperline + x * src->channels;
            posdst = y * dst->bytesperline + x * dst->channels;
            for (tempx = -(kernel / 2); tempx <= kernel / 2; tempx++)
            {
                for (tempy = -(kernel / 2); tempy <= kernel / 2; tempy++)
                {
                    teste = 0;
                    //asdsa maior que 0 e menor que a largura.
                    if (y + tempy < 0 || y + tempy >= src->height || x + tempx < 0 || x + tempx >= src->width)
                        continue;
                    posx = (x + tempx) * src->channels;
                    posy = (y + tempy) * src->bytesperline;
                    pos = posx + posy;
                    if (src->data[pos] == 255)
                    {
                        dst->data[posdst] = 255;
                        teste = 1;
                        break;
                    }
                }
                if (teste == 1)
                    break;
            }
            if (teste == 0)
            {
                dst->data[posdst] = 0;
            }
        }
    }
    return 1;
}

int vc_binary_erode(IVC* src, IVC* dst, int kernel)
{

    if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
        return 0;
    if (src->channels != 1 || dst->channels != 1)
        return 0;

    int x, y, tempx, tempy, posx, posy, teste = 0;
    long int possrc, posdst, pos;

    for (x = 0; x < src->width; x++)
    {
        for (y = 0; y < src->height; y++)
        {
            possrc = y * src->bytesperline + x * src->channels;
            posdst = y * dst->bytesperline + x * dst->channels;
            for (tempx = -(kernel / 2); tempx <= kernel / 2; tempx++)
            {
                for (tempy = -(kernel / 2); tempy <= kernel / 2; tempy++)
                {
                    teste = 0;
                    //asdsa maior que 0 e menor que a largura.
                    if (y + tempy < 0 || y + tempy >= src->height || x + tempx < 0 || x + tempx >= src->width)
                        continue;
                    posx = (x + tempx) * src->channels;
                    posy = (y + tempy) * src->bytesperline;
                    pos = posx + posy;
                    if (src->data[pos] == 0)
                    {
                        dst->data[posdst] = 0;
                        teste = 1;
                        break;
                    }
                }
                if (teste == 1)
                    break;
            }
            if (teste == 0)
            {
                dst->data[posdst] = 255;
            }
        }
    }
    return 1;
}

int vc_binary_open(IVC* src, IVC* dst, int kernelErode, int kernelDilate)
{

    if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
        return 0;
    if (src->channels != 1 || dst->channels != 1)
        return 0;

    IVC* tempImage;
    tempImage = vc_image_new(src->width, src->height, 1, src->levels);
    vc_binary_erode(src, tempImage, kernelErode);
    vc_binary_dilate(tempImage, dst, kernelDilate);
    vc_image_free(tempImage);
    return 1;
}

int vc_binary_close(IVC* src, IVC* dst, int kernelDilate, int kernelErode)
{

    if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
        return 0;
    if (src->channels != 1 || dst->channels != 1)
        return 0;

    IVC* tempImage;
    tempImage = vc_image_new(src->width, src->height, 1, src->levels);
    vc_binary_dilate(src, tempImage, kernelDilate);
    vc_binary_erode(tempImage, dst, kernelErode);
    vc_image_free(tempImage);
    return 1;
}

int get_lowest_label(int labels[4]) {
    int l = 255;
    for (int i = 0; i < 4; i++) {
        if (l >= labels[i] && labels[i] > 0) {
            l = labels[i];
        }
    }
    printf("\n lowest: %d", l);
    return l;
}

int vc_gray_lowpass_mean_filter(IVC* src, IVC* dst, int kernel_size) {
    unsigned char* datasrc = (unsigned char*)src->data;
    int bytesperline_src = src->width * src->channels;
    int channels_src = src->channels;
    unsigned char* datadst = (unsigned char*)dst->data;
    int bytesperline_dst = dst->width * dst->channels;
    int channels_dst = dst->channels;
    int width = src->width;
    int height = src->height;
    int x, y;
    long int pos_src, pos_dst;
    float rf, gf, bf, pixel;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos_src = (y * bytesperline_src) + (x * channels_src);
            pos_dst = (y * bytesperline_dst) + (x * channels_dst);
            pixel = (float)datasrc[pos_src];
            int vmin = 255;
            int vmax = 0;
            for (int ky = y - (kernel_size / 2); ky < y + (kernel_size / 2); ++ky)
            {
                for (int kx = x - (kernel_size / 2); kx < x + (kernel_size / 2); ++kx)
                {
                    if (ky < 0 || ky >= height || kx < 0 || kx >= width)
                    {
                        continue;
                    }
                    int kpos = (ky * bytesperline_dst) + (kx * channels_dst);
                    int kpixel = (int)datasrc[kpos];
                    if (kpixel < vmin)
                    {
                        vmin = kpixel;
                    }
                    else if (kpixel > vmax)
                    {
                        vmax = kpixel;
                    }
                }
            }
            int t = (vmax + vmin) / 2;
            if (t < (int)pixel)
            {
                datadst[pos_dst] = 1;
            }
            else
            {
                datadst[pos_dst] = 0;
            }
        }
    }

}

// Desenha a caixa delimitadora de um objecto
int vc_draw_bounding_box_rgb(IVC* src, IVC* dst, OVC* blobs, int num_blobs, int* min_x, int* max_x, int* min_y, int* max_y)
{
    // Validate input
    if (src->height < 0 || src->width < 0 || (src->levels < 0 || src->levels > 255))
        return 0;
    if (src->channels != 1 || dst->channels != 3)
        return 0;

    int i, x, y;
    long int posdst;

    int x_min = *min_x; // Get minimum x coordinate value
    int x_max = *max_x; // Get maximum x coordinate value
    int y_min = *min_y; // Get minimum y coordinate value
    int y_max = *max_y; // Get maximum y coordinate value

    // Iterate over all blobs
    for (i = 0; i < num_blobs; i++)
    {
        // Check if the blob is within the specified coordinates and width is at least 10 pixels
        if (blobs[i].x >= x_min && blobs[i].x + blobs[i].width <= x_max && blobs[i].width >= 11)//max15
        {
            // Draw the top and bottom borders of the current blob
            for (x = blobs[i].x; x < blobs[i].x + blobs[i].width; x++)
            {
                // Top border
                y = y_min;
                posdst = y * dst->bytesperline + x * dst->channels;
                dst->data[posdst] = 255;     // Red channel
                dst->data[posdst + 1] = 255; // Green channel
                dst->data[posdst + 2] = 255; // Blue channel

                // Bottom border
                y = y_max;
                posdst = y * dst->bytesperline + x * dst->channels;
                dst->data[posdst] = 255;     // Red channel
                dst->data[posdst + 1] = 255; // Green channel
                dst->data[posdst + 2] = 255; // Blue channel
            }

            // Draw the left and right borders of the current blob
            for (y = y_min; y <= y_max; y++)
            {
                // Left border
                x = blobs[i].x;
                posdst = y * dst->bytesperline + x * dst->channels;
                dst->data[posdst] = 255;     // Red channel
                dst->data[posdst + 1] = 255; // Green channel
                dst->data[posdst + 2] = 255; // Blue channel

                // Right border
                x = blobs[i].x + blobs[i].width - 1;
                posdst = y * dst->bytesperline + x * dst->channels;
                dst->data[posdst] = 255;     // Red channel
                dst->data[posdst + 1] = 255; // Green channel
                dst->data[posdst + 2] = 255; // Blue channel
            }
        }
    }

    return 1; // Return success
}



// Desenha o centro de gravidade de um objecto
int vc_draw_centerofgravity(IVC* srcdst, OVC* blob)
{
    int c;
    int x, y;
    int xmin, xmax, ymin, ymax;
    int s = 3;

    xmin = blob->xc - s;
    ymin = blob->yc - s;
    xmax = blob->xc + s;
    ymax = blob->yc + s;

    if (xmin < blob->x) xmin = blob->x;
    if (ymin < blob->y) ymin = blob->y;
    if (xmax > blob->x + blob->width - 1) xmax = blob->x + blob->width - 1;
    if (ymax > blob->y + blob->height - 1) ymax = blob->y + blob->height - 1;

    for (y = ymin; y <= ymax; y++)
    {
        for (c = 0; c < srcdst->channels; c++)
        {
            srcdst->data[y * srcdst->bytesperline + blob->xc * srcdst->channels] = 255;
        }
    }

    for (x = xmin; x <= xmax; x++)
    {
        for (c = 0; c < srcdst->channels; c++)
        {
            srcdst->data[blob->yc * srcdst->bytesperline + x * srcdst->channels] = 255;
        }
    }

    return 1;
}

// Etiquetagem de blobs
// src		: Imagem bin�ria de entrada
// dst		: Imagem grayscale (ir� conter as etiquetas)
// nlabels	: Endere�o de mem�ria de uma vari�vel, onde ser� armazenado o n�mero de etiquetas encontradas.
// OVC*		: Retorna um array de estruturas de blobs (objectos), com respectivas etiquetas. � necess�rio libertar posteriormente esta mem�ria.
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels)
{
    unsigned char* datasrc = (unsigned char*)src->data;
    unsigned char* datadst = (unsigned char*)dst->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int x, y, a, b;
    long int i, size;
    long int posX, posA, posB, posC, posD;
    int labeltable[256] = { 0 };
    int labelarea[256] = { 0 };
    int label = 1; // Etiqueta inicial.
    int num, tmplabel;
    OVC* blobs; // Apontador para array de blobs (objectos) que ser� retornado desta fun��o.

    // Verifica��o de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
    if (channels != 1) return NULL;

    // Copia dados da imagem bin�ria para imagem grayscale
    memcpy(datadst, datasrc, bytesperline * height);

    // Todos os pix�is de plano de fundo devem obrigat�riamente ter valor 0
    // Todos os pix�is de primeiro plano devem obrigat�riamente ter valor 255
    // Ser�o atribu�das etiquetas no intervalo [1,254]
    // Este algoritmo est� assim limitado a 254 labels
    for (i = 0, size = bytesperline * height; i < size; i++)
    {
        if (datadst[i] != 0) datadst[i] = 255;
    }

    // Limpa os rebordos da imagem bin�ria
    for (y = 0; y < height; y++)
    {
        datadst[y * bytesperline + 0 * channels] = 0;
        datadst[y * bytesperline + (width - 1) * channels] = 0;
    }
    for (x = 0; x < width; x++)
    {
        datadst[0 * bytesperline + x * channels] = 0;
        datadst[(height - 1) * bytesperline + x * channels] = 0;
    }

    // Efectua a etiquetagem
    for (y = 1; y < height - 1; y++)
    {
        for (x = 1; x < width - 1; x++)
        {
            // Kernel:
            // A B C
            // D X

            posA = (y - 1) * bytesperline + (x - 1) * channels; // A
            posB = (y - 1) * bytesperline + x * channels; // B
            posC = (y - 1) * bytesperline + (x + 1) * channels; // C
            posD = y * bytesperline + (x - 1) * channels; // D
            posX = y * bytesperline + x * channels; // X

            // Se o pixel foi marcado
            if (datadst[posX] != 0)
            {
                if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0))
                {
                    datadst[posX] = label;
                    labeltable[label] = label;
                    label++;
                }
                else
                {
                    num = 255;

                    // Se A est� marcado
                    if (datadst[posA] != 0) num = labeltable[datadst[posA]];
                    // Se B est� marcado, e � menor que a etiqueta "num"
                    if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
                    // Se C est� marcado, e � menor que a etiqueta "num"
                    if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
                    // Se D est� marcado, e � menor que a etiqueta "num"
                    if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num)) num = labeltable[datadst[posD]];

                    // Atribui a etiqueta ao pixel
                    datadst[posX] = num;
                    labeltable[num] = num;

                    // Actualiza a tabela de etiquetas
                    if (datadst[posA] != 0)
                    {
                        if (labeltable[datadst[posA]] != num)
                        {
                            for (tmplabel = labeltable[datadst[posA]], a = 1; a < label; a++)
                            {
                                if (labeltable[a] == tmplabel)
                                {
                                    labeltable[a] = num;
                                }
                            }
                        }
                    }
                    if (datadst[posB] != 0)
                    {
                        if (labeltable[datadst[posB]] != num)
                        {
                            for (tmplabel = labeltable[datadst[posB]], a = 1; a < label; a++)
                            {
                                if (labeltable[a] == tmplabel)
                                {
                                    labeltable[a] = num;
                                }
                            }
                        }
                    }
                    if (datadst[posC] != 0)
                    {
                        if (labeltable[datadst[posC]] != num)
                        {
                            for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
                            {
                                if (labeltable[a] == tmplabel)
                                {
                                    labeltable[a] = num;
                                }
                            }
                        }
                    }
                    if (datadst[posD] != 0)
                    {
                        if (labeltable[datadst[posD]] != num)
                        {
                            for (tmplabel = labeltable[datadst[posD]], a = 1; a < label; a++)
                            {
                                if (labeltable[a] == tmplabel)
                                {
                                    labeltable[a] = num;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Volta a etiquetar a imagem
    for (y = 1; y < height - 1; y++)
    {
        for (x = 1; x < width - 1; x++)
        {
            posX = y * bytesperline + x * channels; // X

            if (datadst[posX] != 0)
            {
                datadst[posX] = labeltable[datadst[posX]];
            }
        }
    }

    //printf("\nMax Label = %d\n", label);

    // Contagem do n�mero de blobs
    // Passo 1: Eliminar, da tabela, etiquetas repetidas
    for (a = 1; a < label - 1; a++)
    {
        for (b = a + 1; b < label; b++)
        {
            if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
        }
    }
    // Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que n�o hajam valores vazios (zero) entre etiquetas
    if (!*nlabels > 0)
        *nlabels = 0;

    for (a = 1; a < label; a++)
    {
        if (labeltable[a] != 0)
        {
            labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
            (*nlabels)++; // Conta etiquetas
        }
    }

    // Se n�o h� blobs
    if (*nlabels == 0) return NULL;

    // Cria lista de blobs (objectos) e preenche a etiqueta
    blobs = (OVC*)calloc((*nlabels), sizeof(OVC));
    if (blobs != NULL)
    {
        for (a = 0; a < (*nlabels); a++) blobs[a].label = labeltable[a];
    }
    else return NULL;

    return blobs;
}

int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs)
{
    unsigned char* data = (unsigned char*)src->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int x, y, i;
    long int pos;
    int xmin, ymin, xmax, ymax;
    long int sumx, sumy;

    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if (channels != 1) return 0;

    // Conta área de cada blob
    for (i = 0; i < nblobs; i++)
    {
        xmin = width - 1;
        ymin = height - 1;
        xmax = 0;
        ymax = 0;

        sumx = 0;
        sumy = 0;

        blobs[i].area = 0;

        for (y = 1; y < height - 1; y++)
        {
            for (x = 1; x < width - 1; x++)
            {
                pos = y * bytesperline + x * channels;

                if (data[pos] == blobs[i].label)
                {
                    // Área
                    blobs[i].area++;

                    // Centro de Gravidade
                    sumx += x;
                    sumy += y;

                    // Bounding Box
                    if (xmin > x) xmin = x;
                    if (ymin > y) ymin = y;
                    if (xmax < x) xmax = x;
                    if (ymax < y) ymax = y;

                    // Perímetro
                    // Se pelo menos um dos quatro vizinhos não pertence ao mesmo label, então é um pixel de contorno
                    if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label))
                    {
                        blobs[i].perimeter++;
                    }
                }
            }
        }

        // Bounding Box
        blobs[i].x = xmin;
        blobs[i].y = ymin;
        blobs[i].width = (xmax - xmin) + 1;
        blobs[i].height = (ymax - ymin) + 1;

        // Centro de Gravidade
        //blobs[i].xc = (xmax - xmin) / 2;
        //blobs[i].yc = (ymax - ymin) / 2;
        blobs[i].xc = sumx / MAX(blobs[i].area, 1);
        blobs[i].yc = sumy / MAX(blobs[i].area, 1);
    }

    return 1;
}

OVC* vc_get_biggest_blob(OVC* blobs, int nblobs)
{
    OVC* ovc_to_return = NULL;
    for (int i = 0; i < nblobs; i++)
    {
        if (ovc_to_return == NULL) {
            ovc_to_return = &blobs[i];
            continue;
        }
        if (ovc_to_return->area < blobs[i].area)
        {
            ovc_to_return = &blobs[i];
        }
    }
    return ovc_to_return;
}

int RGB_to_BGR(IVC* src)
{
    unsigned char* data = (unsigned char*)src->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int i, size;
    int pos_src;

    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
        return 0;
    if (channels != 3)
        return 0;

    size = width * height * channels;

    for (i = 0; i < size; i += channels)
    {
        pos_src = i;
        int red = data[pos_src];
        data[pos_src] = data[pos_src + 2];
        data[pos_src + 2] = red;
    }

    return 1;
}

// Função para desenhar a bounding box do resistor
int DRAW_RESISTOR_BOX_1(IVC* src, IVC* dst, OVC* blobs, int labels, int video_width, int video_height, int* min_x, int* max_x, int* min_y, int* max_y)
{
    unsigned char* datasrc = (unsigned char*)src->data;
    unsigned char* datadst = (unsigned char*)dst->data;
    int width_src = src->width;
    int height_src = src->height;
    int bytesperline_dst = dst->bytesperline;
    int channels_src = src->channels;
    int channels_dst = dst->channels;
    long int pos_dst, pos_1, pos_2, pos_3, pos_4, pos_5;
    int total_area = 0; // Variable to store the total area of the blobs
    static int count = 0;
    static int previous_crossed = 0; // Flag to determine if the center of mass has crossed the row

    // Error checking
    if ((width_src <= 0) || (height_src <= 0) || (datasrc == NULL)) return 0;
    if (channels_src != 1) return 0;
    if (labels <= 0 || blobs == NULL) return 0;

    // Initialize min and max boundaries
    *min_x = width_src;
    *min_y = height_src;
    *max_x = 0;
    *max_y = 0;

    for (int i = 0; i < labels; i++)
    {
        if (blobs[i].y <= height_src * 0.3)
        {
            if (blobs[i].x < *min_x) *min_x = blobs[i].x;
            if (blobs[i].y < *min_y) *min_y = blobs[i].y;
            if (blobs[i].x + blobs[i].width > *max_x) *max_x = blobs[i].x + blobs[i].width;
            if (blobs[i].y + blobs[i].height > *max_y) *max_y = blobs[i].y + blobs[i].height;

            // Calculate the area of the current blob and add it to the total area
            int blob_area = blobs[i].width * blobs[i].height;
            total_area += blob_area;
        }
    }

    // Only draw the bounding box if the total area is within the specified range
    if (total_area > 600 && total_area < 10000)
    {
        // Calculate the center of mass (centroid) of the bounding box
        int center_x = (*min_x + *max_x) / 2;
        int center_y = (*min_y + *max_y) / 2;

        // Check if the center of mass crosses the specific row
        if (center_y >= 150 && !previous_crossed)
        {
            count++;
            previous_crossed = 1; // Set the flag to indicate crossing has occurred
        }
        else if (center_y < 150)
        {
            previous_crossed = 0; // Reset the flag if center is above the specific row
        }

        // Draw the bounding box around all blobs
        for (int y = *min_y; y < *max_y; y++)
        {
            for (int x = *min_x; x < *max_x; x++)
            {
                pos_dst = y * bytesperline_dst + x * channels_dst;

                if (x == *min_x || x == *max_x - 1 || y == *min_y || y == *max_y - 1)
                {
                    datadst[pos_dst] = 255; // Red for the box
                    datadst[pos_dst + 1] = 0;
                    datadst[pos_dst + 2] = 0;
                }
            }
        }

        // Verify if the border is valid
        if ((*min_y < height_src) && (*max_y > 0))
        {
            int aux_x = *max_x - ((*max_x - *min_x) / 2);
            int aux_y = *max_y - ((*max_y - *min_y) / 2);

            pos_1 = aux_y * bytesperline_dst + aux_x * channels_dst;
            pos_2 = (aux_y + 1) * bytesperline_dst + aux_x * channels_dst;
            pos_3 = aux_y * bytesperline_dst + (aux_x + 1) * channels_dst;
            pos_4 = (aux_y - 1) * bytesperline_dst + aux_x * channels_dst;
            pos_5 = aux_y * bytesperline_dst + (aux_x - 1) * channels_dst;

            datadst[pos_1] = 255; // Red for the box
            datadst[pos_1 + 1] = 0;
            datadst[pos_1 + 2] = 0;

            datadst[pos_2] = 255; // Red for the box
            datadst[pos_2 + 1] = 0;
            datadst[pos_2 + 2] = 0;

            datadst[pos_3] = 255; // Red for the box
            datadst[pos_3 + 1] = 0;
            datadst[pos_3 + 2] = 0;

            datadst[pos_4] = 255; // Red for the box
            datadst[pos_4 + 1] = 0;
            datadst[pos_4 + 2] = 0;

            datadst[pos_5] = 255; // Red for the box
            datadst[pos_5 + 1] = 0;
            datadst[pos_5 + 2] = 0;
        }
    }

    return count;
}


// Função para desenhar a bounding box do resistor
int DRAW_RESISTOR_BOX_2(IVC* src, IVC* dst, OVC* blobs, int labels, int video_width, int video_height, int* min_x, int* max_x, int* min_y, int* max_y)
{
    unsigned char* datasrc = (unsigned char*)src->data;
    unsigned char* datadst = (unsigned char*)dst->data;
    int width_src = src->width;
    int height_src = src->height;
    int bytesperline_dst = dst->bytesperline;
    int channels_src = src->channels;
    int channels_dst = dst->channels;
    long int pos_dst, pos_1, pos_2, pos_3, pos_4, pos_5;
    int aux_x, aux_y;
    int total_area = 0; // Variable to store the total area of the blobs

    // Error checks
    if ((width_src <= 0) || (height_src <= 0) || (datasrc == NULL)) return 0;
    if (channels_src != 1) return 0;
    if (labels <= 0 || blobs == NULL) return 0;

    // Initialize the minimum and maximum limits
    *min_x = width_src;
    *min_y = height_src;
    *max_x = 0;
    *max_y = 0;

    // Find the bounding box that includes all blobs within the specified y range
    for (int i = 0; i < labels; i++)
    {
        if ((blobs[i].y >= height_src * 0.3) && (blobs[i].y <= height_src * 0.65))
        {
            if (blobs[i].x < *min_x) *min_x = blobs[i].x;
            if (blobs[i].y < *min_y) *min_y = blobs[i].y;
            if (blobs[i].x + blobs[i].width > *max_x) *max_x = blobs[i].x + blobs[i].width;
            if (blobs[i].y + blobs[i].height > *max_y) *max_y = blobs[i].y + blobs[i].height;

            // Calculate the area of the current blob and add it to the total area
            int blob_area = blobs[i].width * blobs[i].height;
            total_area += blob_area;
        }
    }

    // Only draw the bounding box if the total area is within the specified range
    if (total_area > 600 && total_area < 10000)
    {
        // Draw the bounding box around all the blobs
        for (int y = *min_y; y < *max_y; y++)
        {
            for (int x = *min_x; x < *max_x; x++)
            {
                pos_dst = y * bytesperline_dst + x * channels_dst;
                if (x == *min_x || x == *max_x - 1 || y == *min_y || y == *max_y - 1)
                {
                    datadst[pos_dst] = 0;       // Red channel
                    datadst[pos_dst + 1] = 0;   // Green channel
                    datadst[pos_dst + 2] = 0;   // Blue channel
                }
            }
        }

        // Draw the center cross only if any blob satisfies the y condition
        if ((*min_y < height_src) && (*max_y > 0))
        {
            aux_x = *min_x + (*max_x - *min_x) / 2;
            aux_y = *min_y + (*max_y - *min_y) / 2;

            pos_1 = aux_y * bytesperline_dst + aux_x * channels_dst;
            pos_2 = (aux_y + 1) * bytesperline_dst + aux_x * channels_dst;
            pos_3 = aux_y * bytesperline_dst + (aux_x + 1) * channels_dst;
            pos_4 = (aux_y - 1) * bytesperline_dst + aux_x * channels_dst;
            pos_5 = aux_y * bytesperline_dst + (aux_x - 1) * channels_dst;

            datadst[pos_1] = 0;
            datadst[pos_1 + 1] = 0;
            datadst[pos_1 + 2] = 0;

            datadst[pos_2] = 0;
            datadst[pos_2 + 1] = 0;
            datadst[pos_2 + 2] = 0;

            datadst[pos_3] = 0;
            datadst[pos_3 + 1] = 0;
            datadst[pos_3 + 2] = 0;

            datadst[pos_4] = 0;
            datadst[pos_4 + 1] = 0;
            datadst[pos_4 + 2] = 0;

            datadst[pos_5] = 0;
            datadst[pos_5 + 1] = 0;
            datadst[pos_5 + 2] = 0;

        }
    }
    return 1;
}

// Função para desenhar a bounding box do resistor
int DRAW_RESISTOR_BOX_3(IVC* src, IVC* dst, OVC* blobs, int labels, int video_width, int video_height, int* min_x, int* max_x, int* min_y, int* max_y)
{
    unsigned char* datasrc = (unsigned char*)src->data;
    unsigned char* datadst = (unsigned char*)dst->data;
    int width_src = src->width;
    int height_src = src->height;
    int bytesperline_dst = dst->bytesperline;
    int channels_src = src->channels;
    int channels_dst = dst->channels;
    long int pos_dst, pos_1, pos_2, pos_3, pos_4, pos_5;
    int aux_x, aux_y;
    int total_area = 0; // Variable to store the total area of the blobs

    // Error checks
    if ((width_src <= 0) || (height_src <= 0) || (datasrc == NULL)) return 0;
    if (channels_src != 1) return 0;
    if (labels <= 0 || blobs == NULL) return 0;

    // Initialize the minimum and maximum limits
    *min_x = width_src;
    *min_y = height_src;
    *max_x = 0;
    *max_y = 0;

    // Find the bounding box that includes all blobs within the specified y range
    for (int i = 0; i < labels; i++)
    {
        if ((blobs[i].y >= height_src * 0.65) && (blobs[i].y <= height_src * 1))
        {
            if (blobs[i].x < *min_x) *min_x = blobs[i].x;
            if (blobs[i].y < *min_y) *min_y = blobs[i].y;
            if (blobs[i].x + blobs[i].width > *max_x) *max_x = blobs[i].x + blobs[i].width;
            if (blobs[i].y + blobs[i].height > *max_y) *max_y = blobs[i].y + blobs[i].height;

            // Calculate the area of the current blob and add it to the total area
            int blob_area = blobs[i].width * blobs[i].height;
            total_area += blob_area;
        }
    }

    // Only draw the bounding box if the total area is within the specified range
    if (total_area > 600 && total_area < 10000)
    {
        // Draw the bounding box around all the blobs
        for (int y = *min_y; y < *max_y; y++)
        {
            for (int x = *min_x; x < *max_x; x++)
            {
                pos_dst = y * bytesperline_dst + x * channels_dst;
                if (x == *min_x || x == *max_x - 1 || y == *min_y || y == *max_y - 1)
                {
                    datadst[pos_dst] = 0;       // Red channel
                    datadst[pos_dst + 1] = 0;   // Green channel
                    datadst[pos_dst + 2] = 0;   // Blue channel
                }
            }
        }

        // Draw the center cross only if any blob satisfies the y condition
        if ((*min_y < height_src) && (*max_y > 0))
        {
            aux_x = *min_x + (*max_x - *min_x) / 2;
            aux_y = *min_y + (*max_y - *min_y) / 2;

            pos_1 = aux_y * bytesperline_dst + aux_x * channels_dst;
            pos_2 = (aux_y + 1) * bytesperline_dst + aux_x * channels_dst;
            pos_3 = aux_y * bytesperline_dst + (aux_x + 1) * channels_dst;
            pos_4 = (aux_y - 1) * bytesperline_dst + aux_x * channels_dst;
            pos_5 = aux_y * bytesperline_dst + (aux_x - 1) * channels_dst;

            datadst[pos_1] = 0;
            datadst[pos_1 + 1] = 0;
            datadst[pos_1 + 2] = 0;

            datadst[pos_2] = 0;
            datadst[pos_2 + 1] = 0;
            datadst[pos_2 + 2] = 0;

            datadst[pos_3] = 0;
            datadst[pos_3 + 1] = 0;
            datadst[pos_3 + 2] = 0;

            datadst[pos_4] = 0;
            datadst[pos_4 + 1] = 0;
            datadst[pos_4 + 2] = 0;

            datadst[pos_5] = 0;
            datadst[pos_5 + 1] = 0;
            datadst[pos_5 + 2] = 0;

        }
    }
    return 1;
}

int vc_color_segmentation(IVC* src, IVC* dst, int max_y, int min_y, int max_x, int min_x)
{
    unsigned char* datasrc = (unsigned char*)src->data;
    int byterperline_src = src->width * src->channels;
    int channels_src = src->channels;
    unsigned char* datadst = (unsigned char*)dst->data;
    int bytesperline_dst = dst->width * dst->channels;
    int channels_dst = dst->channels;
    int width = src->width;
    int height = src->height;
    int x, y;
    long int pos_src, pos_dst;
    float h, s, v;
    

    if (src->width <= 0 || src->height <= 0 || src->data == NULL)
        return 0;
    if (src->width != dst->width || src->height != dst->height)
        return 0;
    if (src->channels != 3 || dst->channels != 1)
        return 0;

    // Segmentation loop
    for (y = min_y; y < max_y; y++)
    {
        for (x = min_x; x < max_x; x++)
        {
            pos_src = y * byterperline_src + x * channels_src;
            pos_dst = y * bytesperline_dst + x * channels_dst;


            // Extract HSV values directly from the src image
            h = ((float)datasrc[pos_src]) / 255.0f * 360.0f;
            s = ((float)datasrc[pos_src + 1]) / 255.0f * 100.0f;
            v = ((float)datasrc[pos_src + 2]) / 255.0f * 100.0f;

            if (h >= 52 && h <= 103 && s >= 30 && s <= 50 && v >= 38 && v <= 63)        // Verde
            {
                datadst[pos_dst] = 254; 
            }
            else if (h >= 127 && h <= 203 && s >= 11 && s <= 39 && v >= 35 && v <= 42)  // Azul
            {
                datadst[pos_dst] = 253; 
            }
            else if (h >= 0 && h <= 15 && s >= 40 && s <= 70 && v >= 60 && v <= 80)     // Vermelho
            {
                datadst[pos_dst] = 252; 
            }
            else if (h >= 5 && h <= 18 && s >= 62 && s <= 80 && v >= 77 && v <= 94)     // Laranja
            {
                datadst[pos_dst] = 251; 
            }
            else if (h >= 31 && h <= 80 && s >= 10 && s <= 36 && v >= 24 && v <= 42)    // Preto
            {
                datadst[pos_dst] = 250; 
            }
            else if (h >= 2 && h <= 36 && s >= 23 && s <= 57 && v >= 33 && v <= 50)     // Castanho
            {
                datadst[pos_dst] = 240; 
            }
            else
            {
                datadst[pos_dst] = 0; // Pixel is outside all ranges, mark as black
            }
        }
    }

    return 1; // Success
}


int DRAW_Color_box1(IVC* src, IVC* dst, OVC* blobs, int labels, int video_width, int video_height, int* min_x, int* max_x, int* min_y, int* max_y)
{
    unsigned char* datasrc = (unsigned char*)src->data;
    unsigned char* datadst = (unsigned char*)dst->data;
    int width_src = src->width;
    int height_src = src->height;
    int bytesperline_dst = dst->bytesperline;
    int channels_src = src->channels;
    int channels_dst = dst->channels;
    long int pos_dst, pos_1, pos_2, pos_3, pos_4, pos_5;
    int aux_x, aux_y;
    int total_area = 0; // Variable to store the total area of the blobs

    // Error checks
    if ((width_src <= 0) || (height_src <= 0) || (datasrc == NULL)) return 0;
    if (channels_src != 1) return 0;
    if (labels <= 0 || blobs == NULL) return 0;

    // Initialize the minimum and maximum limits
    *min_x = width_src;
    *min_y = height_src;
    *max_x = 0;
    *max_y = 0;

    // Find the bounding box that includes all blobs within the specified y range
    for (int i = 0; i < labels; i++)
    {
        if ((blobs[i].y <= height_src * 0.3))
        {
            if (blobs[i].x < *min_x) *min_x = blobs[i].x;
            if (blobs[i].y < *min_y) *min_y = blobs[i].y;
            if (blobs[i].x + blobs[i].width > *max_x) *max_x = blobs[i].x + blobs[i].width;
            if (blobs[i].y + blobs[i].height > *max_y) *max_y = blobs[i].y + blobs[i].height;

            // Calculate the area of the current blob and add it to the total area
            int blob_area = blobs[i].width * blobs[i].height;
            total_area += blob_area;
        }
    }

    // Only draw the bounding box if the total area is within the specified range
   
        // Draw the bounding box around all the blobs
        for (int y = *min_y; y < *max_y; y++)
        {
            for (int x = *min_x; x < *max_x; x++)
            {
                pos_dst = y * bytesperline_dst + x * channels_dst;
                if (x == *min_x || x == *max_x - 1 || y == *min_y || y == *max_y - 1)
                {
                    datadst[pos_dst] = 0;       // Red channel
                    datadst[pos_dst + 1] = 0;   // Green channel
                    datadst[pos_dst + 2] = 0;   // Blue channel
                }
            }
        }

    return 1;
}


OVC* vc_color_calculator(IVC* src, IVC* dst, int* nlabels)
{
    unsigned char* datasrc = (unsigned char*)src->data;
    unsigned char* datadst = (unsigned char*)dst->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int x, y, a, b;
    long int i, size;
    long int posX, posA, posB, posC, posD;
    int labeltable[256] = { 0 };
    int labelarea[256] = { 0 };
    int label = 1; // Etiqueta inicial.
    int num, tmplabel;
    OVC* blobs; // Apontador para array de blobs (objectos) que ser� retornado desta fun��o.

    // Verifica��o de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
    if (channels != 1) return NULL;

    // Copia dados da imagem bin�ria para imagem grayscale
    memcpy(datadst, datasrc, bytesperline * height);

    // Todos os pix�is de plano de fundo devem obrigat�riamente ter valor 0
    // Todos os pix�is de primeiro plano devem obrigat�riamente ter valor 255
    // Ser�o atribu�das etiquetas no intervalo [1,254]
    // Este algoritmo est� assim limitado a 254 labels
    for (i = 0, size = bytesperline * height; i < size; i++)
    {
        if (datadst[i] != 0) datadst[i] = 255;
    }

    // Limpa os rebordos da imagem bin�ria
    for (y = 0; y < height; y++)
    {
        datadst[y * bytesperline + 0 * channels] = 0;
        datadst[y * bytesperline + (width - 1) * channels] = 0;
    }
    for (x = 0; x < width; x++)
    {
        datadst[0 * bytesperline + x * channels] = 0;
        datadst[(height - 1) * bytesperline + x * channels] = 0;
    }

    // Efectua a etiquetagem
    for (y = 1; y < height - 1; y++)
    {
        for (x = 1; x < width - 1; x++)
        {
            // Kernel:
            // A B C
            // D X

            posA = (y - 1) * bytesperline + (x - 1) * channels; // A
            posB = (y - 1) * bytesperline + x * channels; // B
            posC = (y - 1) * bytesperline + (x + 1) * channels; // C
            posD = y * bytesperline + (x - 1) * channels; // D
            posX = y * bytesperline + x * channels; // X

            // Se o pixel foi marcado
            if (datadst[posX] != 0)
            {
                if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0))
                {
                    datadst[posX] = label;
                    labeltable[label] = label;
                    label++;
                }
                else
                {
                    num = 255;

                    // Se A est� marcado
                    if (datadst[posA] != 0) num = labeltable[datadst[posA]];
                    // Se B est� marcado, e � menor que a etiqueta "num"
                    if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
                    // Se C est� marcado, e � menor que a etiqueta "num"
                    if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
                    // Se D est� marcado, e � menor que a etiqueta "num"
                    if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num)) num = labeltable[datadst[posD]];

                    // Atribui a etiqueta ao pixel
                    datadst[posX] = num;
                    labeltable[num] = num;

                    // Actualiza a tabela de etiquetas
                    if (datadst[posA] != 0)
                    {
                        if (labeltable[datadst[posA]] != num)
                        {
                            for (tmplabel = labeltable[datadst[posA]], a = 1; a < label; a++)
                            {
                                if (labeltable[a] == tmplabel)
                                {
                                    labeltable[a] = num;
                                }
                            }
                        }
                    }
                    if (datadst[posB] != 0)
                    {
                        if (labeltable[datadst[posB]] != num)
                        {
                            for (tmplabel = labeltable[datadst[posB]], a = 1; a < label; a++)
                            {
                                if (labeltable[a] == tmplabel)
                                {
                                    labeltable[a] = num;
                                }
                            }
                        }
                    }
                    if (datadst[posC] != 0)
                    {
                        if (labeltable[datadst[posC]] != num)
                        {
                            for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
                            {
                                if (labeltable[a] == tmplabel)
                                {
                                    labeltable[a] = num;
                                }
                            }
                        }
                    }
                    if (datadst[posD] != 0)
                    {
                        if (labeltable[datadst[posD]] != num)
                        {
                            for (tmplabel = labeltable[datadst[posD]], a = 1; a < label; a++)
                            {
                                if (labeltable[a] == tmplabel)
                                {
                                    labeltable[a] = num;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Volta a etiquetar a imagem
    for (y = 1; y < height - 1; y++)
    {
        for (x = 1; x < width - 1; x++)
        {
            posX = y * bytesperline + x * channels; // X

            if (datadst[posX] != 0)
            {
                datadst[posX] = labeltable[datadst[posX]];
            }
        }
    }

    //printf("\nMax Label = %d\n", label);

    // Contagem do n�mero de blobs
    // Passo 1: Eliminar, da tabela, etiquetas repetidas
    for (a = 1; a < label - 1; a++)
    {
        for (b = a + 1; b < label; b++)
        {
            if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
        }
    }
    // Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que n�o hajam valores vazios (zero) entre etiquetas
    if (!*nlabels > 0)
        *nlabels = 0;

    for (a = 1; a < label; a++)
    {
        if (labeltable[a] != 0)
        {
            labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
            (*nlabels)++; // Conta etiquetas
        }
    }

    // Se n�o h� blobs
    if (*nlabels == 0) return NULL;

    // Cria lista de blobs (objectos) e preenche a etiqueta
    blobs = (OVC*)calloc((*nlabels), sizeof(OVC));
    if (blobs != NULL)
    {
        for (a = 0; a < (*nlabels); a++) blobs[a].label = labeltable[a];
    }
    else return NULL;

    return blobs;
}
