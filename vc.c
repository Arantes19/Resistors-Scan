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

int vc_rgb_to_hsv(IVC* src, IVC* dst)
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
    float rf, gf, bf, Max, Min, H = 0, S = 0, V = 0;

    // Verifica��o de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
        return 0;

    if ((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL))
        return 0;

    if ((src->channels != 3))
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

            Max = getMax(rf, gf, bf);
            Min = getMin(rf, gf, bf);

            V = Max;
            if (Max - Min == 0 || V == 0)
            {
                S = 0;
            }
            else
            {
                S = (Max - Min) / V;
            }

            if (rf == Max)
            {
                if (gf >= bf)
                    H = 60 * (gf - bf) / (Max - Min);
                else
                    H = 360 + 60 * (gf - bf) / (Max - Min);
            }
            else if (gf == Max)
                H = 120 + 60 * (bf - rf) / (Max - Min);
            else if (bf == Max)
                H = 240 + 60 * (bf - rf) / (Max - Min);

            H = H * 255 / 360;
            S = S * 255;
            datadst[pos_dst] = (unsigned char)(H);
            datadst[pos_dst + 1] = (unsigned char)(S);
            datadst[pos_dst + 2] = (unsigned char)(V);
        }
    }

    return 1;
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

int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax)
{
    if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
        return 0;
    if (src->channels != 3 || dst->channels != 1)
        return 0;

    int x, y;
    long int possrc, posdst;

    for (y = 0; y < src->height; y++)
    {
        for (x = 0; x < src->width; x++)
        {
            possrc = y * src->bytesperline + x * src->channels;
            posdst = y * dst->bytesperline + x * dst->channels;

            if ((src->data[possrc] >= hmin && src->data[possrc] <= hmax) &&
                (src->data[possrc + 1] >= smin && src->data[possrc + 1] <= smax) &&
                (src->data[possrc + 2] >= vmin && src->data[possrc + 2] <= vmax))
            {
                dst->data[posdst] = 255;
            }
            else
            {
                dst->data[posdst] = 0;
            }
        }
    }

    return 1;
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



// Convers�o de FLIR para RGB
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

int vc_binary_dilate(IVC* src, IVC* dst, int kernel_size)
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
    int should_dilate = 0;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos_src = (y * bytesperline_src) + (x * channels_src);
            pos_dst = (y * bytesperline_dst) + (x * channels_dst);
            pixel = (float)datasrc[pos_src];
            should_dilate = 0;
            int kpixel = 0;
            for (int ky = y - (kernel_size / 2); ky < y + (kernel_size / 2); ++ky)
            {
                for (int kx = x - (kernel_size / 2); kx < x + (kernel_size / 2); ++kx)
                {
                    if (ky < 0 || ky >= height || kx < 0 || kx >= width)
                    {
                        continue;
                    }
                    // if (ky==(y * bytesperline_src) && kx==(x * channels_src))
                    //{
                    //     continue;
                    // }

                    int kpos = (ky * bytesperline_dst) + (kx * channels_dst);
                    kpixel = (int)datasrc[kpos];
                    if (kpixel == 1)
                    {
                        should_dilate = 1;
                        break;
                    }
                }
                if (should_dilate == 1)
                {
                    break;
                }
            }
            if (should_dilate == 1)
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

int vc_binary_erode(IVC* src, IVC* dst, int kernel_size)
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
    int should_erode = 0;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos_src = (y * bytesperline_src) + (x * channels_src);
            pos_dst = (y * bytesperline_dst) + (x * channels_dst);
            pixel = (float)datasrc[pos_src];
            char valor = (float)datasrc[pos_src];
            char valor_dst = (float)datadst[pos_src];
            //printf(" %lu -", valor);
        }
        //printf("\n");
    }

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos_src = (y * bytesperline_src) + (x * channels_src);
            pos_dst = (y * bytesperline_dst) + (x * channels_dst);
            pixel = (float)datasrc[pos_src];
            should_erode = 0;
            int kpixel = 0;
            for (int ky = y - (kernel_size / 2); ky < y + (kernel_size / 2); ++ky)
            {
                for (int kx = x - (kernel_size / 2); kx < x + (kernel_size / 2); ++kx)
                {
                    if (ky < 0 || ky >= height || kx < 0 || kx >= width)
                    {
                        continue;
                    }
                    // if (ky==(y * bytesperline_src) && kx==(x * channels_src))
                    //{
                    //     continue;
                    // }

                    int kpos = (ky * bytesperline_dst) + (kx * channels_dst);
                    kpixel = (int)datasrc[kpos];
                    if (kpixel == 0)
                    {
                        should_erode = 1;
                        break;
                    }
                }
                if (should_erode == 1)
                {
                    break;
                }
            }
            if (should_erode == 1)
            {
                datadst[pos_dst] = 0;
            }
            else
            {
                datadst[pos_dst] = 1;
            }
        }
    }
}

int vc_binary_open(IVC* src, IVC* dst, int kernelerode, int kerneldilate)
{
    IVC* temp;
    temp = vc_image_new(src->width, src->height, 1, 255); // Creates an empty image with the resolution of the src image
    if (temp == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        return 0;
    }
    vc_binary_erode(src, temp, kernelerode);
    vc_binary_dilate(temp, dst, kerneldilate);

    vc_image_free(temp);

    return 1;
}

int vc_binary_close(IVC* src, IVC* dst, int kerneldilate, int kernelerode)
{
    IVC* temp;
    temp = vc_image_new(src->width, src->height, 1, 255); // Creates an empty image with the resolution of the src image
    if (temp == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        return 0;
    }
    vc_binary_dilate(src, temp, kerneldilate);
    vc_binary_erode(temp, dst, kernelerode);

    vc_image_free(temp);

    return 1;
}


int vc_binary_blob_labelling_2(IVC* src, IVC* dst)
{
    // info source
    unsigned char* datasrc = (unsigned char*)src->data;
    int bytesperline_src = src->width * src->channels;
    int channels_src = src->channels;

    //info destino
    unsigned char* datadst = (unsigned char*)dst->data;
    int bytesperline_dst = dst->width * dst->channels;
    int channels_dst = dst->channels;

    // medidas
    int width = src->width;
    int height = src->height;

    //outras variaveis
    int x, y;
    long int pos_A, pos_B, pos_C, pos_D, pos_X;
    int label = 10;
    int tmplabel, a, b;
    int labeltable[255];
    int num;

    //verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height)) return 0;
    if ((src->channels != 1) || (dst->channels != 1)) return 0;

    for (y = 0; y < height; y++)
    {

        for (x = 0; x < width; x++)
        {
            pos_X = y * bytesperline_src + x * channels_src;
            pos_A = (y - 1) * bytesperline_src + (x - 1) * channels_src;
            pos_B = (y - 1) * bytesperline_src + (x)*channels_src;
            pos_C = (y - 1) * bytesperline_src + (x + 1) * channels_src;
            pos_D = (y)*bytesperline_src + (x - 1) * channels_src;

            if (datasrc[pos_X] == 1)
            {

                if ((datadst[pos_A] == 0) && (datadst[pos_B] == 0) && (datadst[pos_C] == 0) && (datadst[pos_D] == 0))
                {
                    datadst[pos_X] = label;
                    labeltable[label] = label;
                    label += 20;
                }
                else
                {
                    int vizinhos[4] = { datadst[pos_A],datadst[pos_B],datadst[pos_C],datadst[pos_D] };
                    int lowest_label = get_lowest_label(vizinhos);
                    datadst[pos_X] = lowest_label;

                    // Actualiza a tabela de etiquetas
                    if (datadst[pos_A] != 0)
                    {
                        if (labeltable[datadst[pos_A]] != lowest_label)
                        {
                            for (tmplabel = labeltable[datadst[pos_A]], a = 1; a < label; a++)
                            {
                                if (labeltable[a] == tmplabel)
                                {
                                    labeltable[a] = lowest_label;
                                }
                            }
                        }
                    }
                    if (datadst[pos_B] != 0)
                    {
                        if (labeltable[datadst[pos_B]] != lowest_label)
                        {
                            for (tmplabel = labeltable[datadst[pos_B]], a = 1; a < label; a++)
                            {
                                if (labeltable[a] == tmplabel)
                                {
                                    labeltable[a] = lowest_label;
                                }
                            }
                        }
                    }
                    if (datadst[pos_C] != 0)
                    {
                        if (labeltable[datadst[pos_C]] != lowest_label)
                        {
                            for (tmplabel = labeltable[datadst[pos_C]], a = 1; a < label; a++)
                            {
                                if (labeltable[a] == tmplabel)
                                {
                                    labeltable[a] = lowest_label;
                                }
                            }
                        }
                    }
                    if (datadst[pos_D] != 0)
                    {
                        if (labeltable[datadst[pos_D]] != lowest_label)
                        {
                            for (tmplabel = labeltable[datadst[pos_D]], a = 1; a < label; a++)
                            {
                                if (labeltable[a] == tmplabel)
                                {
                                    labeltable[a] = lowest_label;
                                }
                            }
                        }
                    }
                }

            }
            else {
                datadst[pos_X] = 0;
            }
        }
    }
    //Volta a etiquetar a imagem
    for (y = 1; y < height - 1; y++)
    {
        for (x = 1; x < width - 1; x++)
        {
            pos_X = y * bytesperline_src + x * channels_src;
            if (datadst[pos_X] != 0)
            {
                datadst[pos_X] = labeltable[datadst[pos_X]];
            }
        }
    }
    return 0;
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
int vc_draw_boundingbox(IVC* srcdst, OVC* blob)
{
    int c;
    int x, y;

    for (y = blob->y; y < blob->y + blob->height; y++)
    {
        for (c = 0; c < srcdst->channels; c++)
        {
            srcdst->data[y * srcdst->bytesperline + blob->x * srcdst->channels] = 255;
            srcdst->data[y * srcdst->bytesperline + (blob->x + blob->width - 1) * srcdst->channels] = 255;
        }
    }

    for (x = blob->x; x < blob->x + blob->width; x++)
    {
        for (c = 0; c < srcdst->channels; c++)
        {
            srcdst->data[blob->y * srcdst->bytesperline + x * srcdst->channels] = 255;
            srcdst->data[(blob->y + blob->height - 1) * srcdst->bytesperline + x * srcdst->channels] = 255;
        }
    }

    return 1;
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
// src		: Imagem bin�ria
// dst		: Imagem grayscale (irá conter as etiquetas)
// nlabels	: Endere�o de mem�ria de uma variável inteira. Recebe o n�mero de etiquetas encontradas.
// OVC*		: Retorna lista de estruturas de blobs (objectos), com respectivas etiquetas. � necess�rio libertar posteriormente esta mem�ria.
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
    int labeltable[5000] = { 0 };
    int labelarea[5000] = { 0 };
    int label = 1; // Etiqueta inicial.
    int num, tmplabel;
    OVC* blobs; // Apontador para lista de blobs (objectos) que ser� retornada desta fun��o.

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
        for (a = 0; a < (*nlabels); a++) {

            blobs[a].label = labeltable[a];
        }
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

IVC *ONE_CHANNEL_VISUALIZER(IVC *src_image)
{
    IVC *one_channel_image = vc_image_new(src_image->width, src_image->height, 3, 255);
    vc_gray_3channels(src_image, one_channel_image); 

    return one_channel_image;
}
