/*********************************************************************/
//
//	brief: This file provides an easy way to obtain image data information.
//
//
//	BINARY_INFO[PICTURE_NAME].number      => To obtain sequence number information of the "PICTURE_NAME"(.bmp or .jpg) which is generated in the excel file
//	BINARY_INFO[PICTURE_NAME].img_width       => To obtain image width information of the "PICTURE_NAME" (.bmp or .jpg)
//	BINARY_INFO[PICTURE_NAME].img_height      => To obtain image height information of the "PICTURE_NAME" (.bmp or .jpg)
//	BINARY_INFO[PICTURE_NAME].img_size        => To obtain an image size which is converted and stored in the binary file, please notice, the binary file is combined and converted from the several files (.bmp or .jpg)
//	BINARY_INFO[PICTURE_NAME].start_addr  => To obtain start address of binary file which is converted from the file "PICTURE_NAME" (.bmp or .jpg)
//
//	For example, the struct and enum type as:
//
//	const INFO (code) BINARY_INFO[3]=
//	{
//  	/*  No. , Width , Height , Size (bytes) , Start Address  */
//  	{1,1280,800,2048000,0},          /*     pic_01_1280x800 , element 0     */
//  	{2,320,240,153600,2048000},      /*     RAiO , element 1     */
//  	{3,128,128,32768,2201600},       /*     S1_16 , element 2     */
//	}
//
//
//  typedef enum
//  {
//    pic_01_1280x800=0,  /*     0     */
//    RAiO,               /*     1     */
//    S1_16,              /*     2     */
//  }PICTURE_NAME;
//
//	(1).  To obtain size informations of the file "pic_01_1280x800" (.bmp or jpg),
//        the C code is:
//
//									long param1 = 0;
//
//									param1 = BINARY_INFO[pic_01_1280x800].img_size;
//									/*  or  */
//									param1 = BINARY_INFO[0].img_size;
//
//									/* the param1 is 2048000 (bytes) */
//
//
//	(2).  To obtain start address informations of the file "S1_16" (.bmp or jpg),
//        the C code is:
//
//									long param2 = 0;
//
//									param2 = BINARY_INFO[S1_16].start_addr;
//									/*  or  */
//									param2 = BINARY_INFO[2].start_addr;
//
//									/* the param2 is 2201600 (bytes) */
//
/*********************************************************************/
typedef struct _info
{
  unsigned short number;
  unsigned short img_width;
  unsigned short img_height;
  unsigned long img_size;
  unsigned long start_addr;
}INFO;

  /* The 'code' is KEIL C 8051 instruction, please refer to http://www.keil.com/support/man/docs/c51/c51_le_code.htm */
  /* If you do not use the 8051 microcontroller system, please remove the 'code' instruction. */

const INFO code BINARY_INFO[24]=
{
  /*  No. , Width , Height , Size , Start Address  */ 
  {1,108,216,46656,0},          /*     char0020 , element 0     */
  {2,108,216,46656,46656},          /*     char0030 , element 1     */
  {3,108,216,46656,93312},          /*     char0031 , element 2     */
  {4,108,216,46656,139968},          /*     char0032 , element 3     */
  {5,108,216,46656,186624},          /*     char0033 , element 4     */
  {6,108,216,46656,233280},          /*     char0034 , element 5     */
  {7,108,216,46656,279936},          /*     char0035 , element 6     */
  {8,108,216,46656,326592},          /*     char0036 , element 7     */
  {9,108,216,46656,373248},          /*     char0037 , element 8     */
  {10,108,216,46656,419904},          /*     char0038 , element 9     */
  {11,108,216,46656,466560},          /*     char0039 , element 10     */
  {12,108,216,46656,513216},          /*     char0020_mask , element 11     */
  {13,108,216,46656,559872},          /*     char0030_mask , element 12     */
  {14,108,216,46656,606528},          /*     char0031_mask , element 13     */
  {15,108,216,46656,653184},          /*     char0032_mask , element 14     */
  {16,108,216,46656,699840},          /*     char0033_mask , element 15     */
  {17,108,216,46656,746496},          /*     char0034_mask , element 16     */
  {18,108,216,46656,793152},          /*     char0035_mask , element 17     */
  {19,108,216,46656,839808},          /*     char0036_mask , element 18     */
  {20,108,216,46656,886464},          /*     char0037_mask , element 19     */
  {21,108,216,46656,933120},          /*     char0038_mask , element 20     */
  {22,108,216,46656,979776},          /*     char0039_mask , element 21     */
  {23,1280,720,1843200,1026432},          /*     background_1 , element 22     */
  {24,1280,720,1843200,2869632},          /*     background_2 , element 23     */
};

typedef enum
{
  char0020=0,          /*     0     */
  char0030,          /*     1     */
  char0031,          /*     2     */
  char0032,          /*     3     */
  char0033,          /*     4     */
  char0034,          /*     5     */
  char0035,          /*     6     */
  char0036,          /*     7     */
  char0037,          /*     8     */
  char0038,          /*     9     */
  char0039,          /*     10     */
  char0020_mask,          /*     11     */
  char0030_mask,          /*     12     */
  char0031_mask,          /*     13     */
  char0032_mask,          /*     14     */
  char0033_mask,          /*     15     */
  char0034_mask,          /*     16     */
  char0035_mask,          /*     17     */
  char0036_mask,          /*     18     */
  char0037_mask,          /*     19     */
  char0038_mask,          /*     20     */
  char0039_mask,          /*     21     */
  background_1,          /*     22     */
  background_2,          /*     23     */
}PICTURE_NAME;

