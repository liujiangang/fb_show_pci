#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include <linux/mxcfb.h> 

//14byte文件头
typedef struct
{
	char cfType[2];//文件类型，"BM"(0x4D42)
	long cfSize;//文件大小（字节）
	long cfReserved;//保留，值为0
	long cfoffBits;//数据区相对于文件头的偏移量（字节）
}__attribute__((packed)) BITMAPFILEHEADER;
//__attribute__((packed))的作用是告诉编译器取消结构在编译过程中的优化对齐

/*
//40byte信息头
typedef struct
{
	char ciSize[4];//BITMAPFILEHEADER所占的字节数
	long ciWidth;//宽度
	long ciHeight;//高度
	char ciPlanes[2];//目标设备的位平面数，值为1
	int ciBitCount;//每个像素的位数
	char ciCompress[4];//压缩说明
	char ciSizeImage[4];//用字节表示的图像大小，该数据必须是4的倍数
	char ciXPelsPerMeter[4];//目标设备的水平像素数/米
	char ciYPelsPerMeter[4];//目标设备的垂直像素数/米
	char ciClrUsed[4]; //位图使用调色板的颜色数
	char ciClrImportant[4]; //指定重要的颜色数，当该域的值等于颜色数时（或者等于0时），表示所有颜色都一样重要
}__attribute__((packed)) BITMAPINFOHEADER;
*/

//40byte信息头
typedef struct
{
	long ciSize;//BITMAPFILEHEADER所占的字节数
	long ciWidth;//宽度
	long ciHeight;//高度
	char ciPlanes[2];//目标设备的位平面数，值为1
	int ciBitCount;//每个像素的位数
	long ciCompress;//压缩说明
	long ciSizeImage;//用字节表示的图像大小，该数据必须是4的倍数
	char ciXPelsPerMeter[4];//目标设备的水平像素数/米
	char ciYPelsPerMeter[4];//目标设备的垂直像素数/米
	long ciClrUsed; //位图使用调色板的颜色数
	char ciClrImportant[4]; //指定重要的颜色数，当该域的值等于颜色数时（或者等于0时），表示所有颜色都一样重要
}__attribute__((packed)) BITMAPINFOHEADER;

typedef struct
{
	unsigned short blue;
	unsigned short green;
	unsigned short red;
	unsigned short reserved;
}__attribute__((packed)) PIXEL;//颜色模式RGB

BITMAPFILEHEADER FileHead;
BITMAPINFOHEADER InfoHead;
PIXEL pixel;

static char *fbp = 0;
static int xres = 0;
static int yres = 0;
static int bits_per_pixel = 0;
static int line_length = 0;
static int yoffset = 0;

int show_bmp();

int main(int argc, char **argv)
{
	char fb_device[100] = "/dev/graphics/fb0";

	
	
	int fbfd = 0;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	long int screensize = 0;
	struct fb_bitfield red;
	struct fb_bitfield green;
	struct fb_bitfield blue;

	//打开显示设备
	fbfd = open(fb_device, O_RDWR);
	if (!fbfd)
	{
		printf("Error: cannot open framebuffer device.\n");
		exit(1);
	}

	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo))
	{
		printf("Error：reading fixed information.\n");
		exit(2);
	}

	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo))
	{
		printf("Error: reading variable information.\n");
		exit(3);
	}
	
	printf("vinfo.xres=%d\n",vinfo.xres);   //屏幕宽
	printf("vinfo.yres=%d\n",vinfo.yres);   //屏幕高
	printf("vinfo.xoffset=%d\n",vinfo.xoffset); //当前屏幕的x偏移
  printf("vinfo.yoffset=%d\n",vinfo.yoffset); //当前屏幕的y偏移，android是双缓冲或者是三缓冲，一帧数据包包含两个屏的数据，或者三个屏的数据
  
  printf("vinfo.xres_virtual=%d\n",vinfo.xres_virtual);   //一帧数据的屏幕宽
	printf("vinfo.yres_virtual=%d\n",vinfo.yres_virtual);   //一帧数据的屏幕高，一般等于2*yres（双缓冲时）
  
  printf("vinfo.bits_per_pixel=%d\n",vinfo.bits_per_pixel);
  
  printf("vinfo.red.offset=%d\n",vinfo.red.offset);   //r在bits的偏移量
  printf("vinfo.red.length=%d\n",vinfo.red.length);   //r在bits中有多少个bit
  printf("vinfo.red.msb_right=%d\n",vinfo.red.msb_right); //0 代表Big endian 
  printf("vinfo.green.offset=%d\n",vinfo.green.offset);
  printf("vinfo.green.length=%d\n",vinfo.green.length);
  printf("vinfo.green.msb_right=%d\n",vinfo.green.msb_right);
  printf("vinfo.blue.offset=%d\n",vinfo.blue.offset);
  printf("vinfo.blue.length=%d\n",vinfo.blue.length);
  printf("vinfo.blue.msb_right=%d\n",vinfo.blue.msb_right);
  
  
  printf("finfo.type=%d\n",finfo.type); //大多数Android设备的 fb0 都应该是 type == 0 的，type 为0 意思说 Framebuffer 里头存的是每个像素点的 ARGB 信息。
  
  //其实我们的 Framebuffer 里头保存的一屏数据并不一定刚好就是我们的屏幕分辨率大小，它的横向像素值有可能比屏幕的横向像素值多！假如你发现你截出来的图片屏幕外有黑边，那原因就在这里了。
  printf("finfo.line_length=%d\n",finfo.line_length);  //Framebuffer里头的图像数据的一行，不应该是 屏幕横向分辨率 * 4，而应该是 line_length (它的单位不是像素点，而是字节) 
  
	xres = vinfo.xres;
	yres = vinfo.yres;
	bits_per_pixel = vinfo.bits_per_pixel;
	yoffset = vinfo.yoffset;
	line_length = finfo.line_length;
	
	
	//计算屏幕的总大小（字节）
	screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
	printf("screensize=%d byte\n",screensize);


	struct mxcfb_gbl_alpha gbl_alpha;
	gbl_alpha.enable = 1; 
  gbl_alpha.alpha = 64; 
  int ret = ioctl(fbfd, MXCFB_SET_GBL_ALPHA, &gbl_alpha);
	if(ret <0) { 
    printf("Error!MXCFB_SET_GBL_ALPHA failed!"); 
    return -1; 
	}
	struct mxcfb_color_key key;
	key.enable = 1;
	key.color_key = 0x00000000; // Black
	ret = ioctl(fbfd, MXCFB_SET_CLR_KEY, &key);
	if(ret <0) {
	    printf("Error!Colorkey setting failed for dev ");
	    return -1;
	}

	//对象映射
	fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
	if ((int)fbp == -1)
	{
		printf("Error: failed to map framebuffer device to memory.\n");
		exit(4);
	}
	memset(fbp,0,screensize); 
	
	printf("sizeof file header=%d\n", sizeof(BITMAPFILEHEADER));

	printf("into show_bmp function\n");

	//显示图像
	int res = show_bmp();
	printf("into show_bmp function res = %d\n", res);
	
/*	int x = 0;
	int y = 0;
	long location = 0;
	for(x=0;x<vinfo.xres;x++)
  	for(y=0;y<vinfo.yres;y++){
       location =(x+vinfo.xoffset)*(vinfo.bits_per_pixel/8)+(y+vinfo.yoffset)*finfo.line_length;
       *(fbp+location+0)=0x00; //Red
       *(fbp+location+1)=0x00; //Green
       *(fbp+location+2)=0xFF; //Blue
       *(fbp+location+3)=0x22;
    }   
  sleep(10); 
  */
    
	//删除对象映射
	munmap(fbp, screensize);
	close(fbfd);
	
	while(1)
	{
		;	
	}
	
	
	return 0;
}


int show_bmp()
{
	FILE *fp;
	int rc;
	int line_x, line_y;
	long int location = 0, BytesPerLine = 0;
	char tmp[1024*10];

	fp = fopen( "/sdcard/Images_32bit.bmp", "rb" );
	if (fp == NULL)
	{
		return( -1 );
	}

	rc = fread( &FileHead, sizeof(BITMAPFILEHEADER),1, fp );
	if ( rc != 1)
	{
		printf("read header error!\n");
		fclose( fp );
		return( -2 );
	}
	printf("FileHead.cfSize = %d \n", FileHead.cfSize);
	printf("FileHead.cfoffBits = %d \n", FileHead.cfoffBits);
	
	//检测是否是bmp图像
	if (memcmp(FileHead.cfType, "BM", 2) != 0)
	{
		printf("it's not a BMP file\n");
		fclose( fp );
		return( -3 );
	}

	rc = fread( (char *)&InfoHead, sizeof(BITMAPINFOHEADER),1, fp );
	if ( rc != 1)
	{
		printf("read infoheader error!\n");
		fclose( fp );
		return( -4 );
	}
	printf("InfoHead.ciSize = %d \n", (int)InfoHead.ciSize);
	printf("InfoHead.ciWidth = %d \n", (int)InfoHead.ciWidth);
	printf("InfoHead.ciHeight = %d \n", (int)InfoHead.ciHeight);
	printf("InfoHead.ciBitCount = 0x%x\n",(int)InfoHead.ciBitCount);
	printf("InfoHead.ciCompress = 0x%x\n",(int)InfoHead.ciCompress);
	printf("InfoHead.ciSizeImage = 0x%x\n",(int)InfoHead.ciSizeImage);
	
	
	//跳转的数据区
	fseek(fp, FileHead.cfoffBits, SEEK_SET);
	//每行字节数
	BytesPerLine = (InfoHead.ciWidth * InfoHead.ciBitCount + 31) / 32 * 4;

	line_x = line_y = 0;
	//向framebuffer中写BMP图片
	int fbp_offset = (yoffset)*line_length;
	while(!feof(fp))
	{
		PIXEL pix;
		unsigned short int tmp;
		rc = fread( (char *)&pix, 1, sizeof(PIXEL), fp);
		
		if (rc != sizeof(PIXEL))
			break;
		//location = line_x * bits_per_pixel / 8 + (InfoHead.ciHeight - line_y - 1) * xres * bits_per_pixel / 8;
		
		location = line_x * bits_per_pixel / 8 + (InfoHead.ciHeight - line_y - 1) * line_length;
		

		//显示每一个像素
		*(fbp + fbp_offset + location + 0)=pix.red;
		*(fbp + fbp_offset + location + 1)=pix.green;
		*(fbp + fbp_offset + location + 2)=pix.blue;
		*(fbp + fbp_offset + location + 3)=pix.reserved;

		line_x++;
		if (line_x == InfoHead.ciWidth )
		{
			line_x = 0;
			line_y++;
			if(line_y == InfoHead.ciHeight)
				break;
		}
		
		
	}
	
	
	
	
	
	fclose( fp );
	return( 0 );
}


