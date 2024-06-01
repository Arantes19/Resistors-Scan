#pragma once
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT?CNICO DO C?VADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORM?TICOS
//                    VIS?O POR COMPUTADOR
//
//             [  BRUNO OLIVEIRA - boliveira@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#define VC_DEBUG


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct {
	int x, y, width, height; // Caixa Delimitadora (Bounding Box)
	int area; // �rea
	int xc, yc; // Centro-de-massa
	int perimeter; // Perimetro
	int label; // Etiqueta
} OVC;

typedef struct {
	unsigned char* data;
	int width, height;
	int channels;			// Bin?rio/Cinzentos=1; RGB=3
	int levels;				// Bin?rio=1; Cinzentos [1,255]; RGB [1,255]
	int bytesperline;		// width * channels
} IVC;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    PROT?TIPOS DE FUN??ES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// FUN??ES: ALOCAR E LIBERTAR UMA IMAGEM
IVC* vc_image_new(int width, int height, int channels, int levels);
IVC* vc_image_free(IVC* image);

// FUN??ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
IVC* vc_read_image(char* filename);
int vc_write_image(char* filename, IVC* image);
int vc_gray_negative(IVC* srcdst);
int vc_rgb_negative(IVC* srcdst);
int vc_rgb_get_red_gray(IVC* srcdst);
int vc_rgb_get_green_gray(IVC* srcdst);
int vc_rgb_get_blue_gray(IVC* srcdst);
int vc_rgb_to_gray(IVC* src, IVC* dst);
int vc_rgb_to_hsv(IVC* src, IVC* dst);
int vc_rgb_to_hsv2(IVC* srcdst);
IVC* vc_convert_bgr_to_rgb(IVC* src); 
int vc_hsv_segmentation(IVC* src, IVC* dst);
int vc_gray_3channels(IVC* src, IVC* dst);
int vc_scale_gray_to_rgb(IVC* src, IVC* dst);
int vc_gray_to_binary(IVC* src, IVC* dst, int threshold);
int vc_gray_to_binary_midpoint(IVC* src, IVC* dst, int kernel_size);
int vc_binary_dilate(IVC* src, IVC* dst, int kernel);
int vc_binary_erode(IVC* src, IVC* dst, int kernel);
int vc_binary_open(IVC* src, IVC* dst, int kernelerode, int kerneldilate);
int vc_binary_close(IVC* src, IVC* dst, int kernelDilate, int kernelErode);

OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels);
int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs);
int rgb_to_bgr(IVC* src);
int draw_resistor_box_1(IVC* src, IVC* dst, OVC* blobs, int labels, int video_width, int video_height, int* min_x, int* main_y, int* max_x, int* max_y);
int draw_resistor_box_2(IVC* src, IVC* dst, OVC* blobs, int labels, int video_width, int video_height, int* min_x, int* main_y, int* max_x, int* max_y, int count);
int draw_resistor_box_3(IVC* src, IVC* dst, OVC* blobs, int labels, int video_width, int video_height, int* min_x, int* main_y, int* max_x, int* max_y, int count);
int vc_color_segmentation(IVC* src, IVC* dst, int max_y, int min_y, int max_x, int min_x);
int vc_color_calculator(IVC* src, IVC* dst, int* nlabels, int* min_x, int* max_x, int* min_y, int* max_y, int value);

