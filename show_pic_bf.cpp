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

//14byte�ļ�ͷ
typedef struct
{
	char cfType[2];//�ļ����ͣ�"BM"(0x4D42)
	long cfSize;//�ļ���С���ֽڣ�
	long cfReserved;//������ֵΪ0
	long cfoffBits;//������������ļ�ͷ��ƫ�������ֽڣ�
}__attribute__((packed)) BITMAPFILEHEADER;
//__attribute__((packed))�������Ǹ��߱�����ȡ���ṹ�ڱ�������е��Ż�����

/*
//40byte��Ϣͷ
typedef struct
{
	char ciSize[4];//BITMAPFILEHEADER��ռ���ֽ���
	long ciWidth;//���
	long ciHeight;//�߶�
	char ciPlanes[2];//Ŀ���豸��λƽ������ֵΪ1
	int ciBitCount;//ÿ�����ص�λ��
	char ciCompress[4];//ѹ��˵��
	char ciSizeImage[4];//���ֽڱ�ʾ��ͼ���С�������ݱ�����4�ı���
	char ciXPelsPerMeter[4];//Ŀ���豸��ˮƽ������/��
	char ciYPelsPerMeter[4];//Ŀ���豸�Ĵ�ֱ������/��
	char ciClrUsed[4]; //λͼʹ�õ�ɫ�����ɫ��
	char ciClrImportant[4]; //ָ����Ҫ����ɫ�����������ֵ������ɫ��ʱ�����ߵ���0ʱ������ʾ������ɫ��һ����Ҫ
}__attribute__((packed)) BITMAPINFOHEADER;
*/

//40byte��Ϣͷ
typedef struct
{
	long ciSize;//BITMAPFILEHEADER��ռ���ֽ���
	long ciWidth;//���
	long ciHeight;//�߶�
	char ciPlanes[2];//Ŀ���豸��λƽ������ֵΪ1
	int ciBitCount;//ÿ�����ص�λ��
	long ciCompress;//ѹ��˵��
	long ciSizeImage;//���ֽڱ�ʾ��ͼ���С�������ݱ�����4�ı���
	char ciXPelsPerMeter[4];//Ŀ���豸��ˮƽ������/��
	char ciYPelsPerMeter[4];//Ŀ���豸�Ĵ�ֱ������/��
	long ciClrUsed; //λͼʹ�õ�ɫ�����ɫ��
	char ciClrImportant[4]; //ָ����Ҫ����ɫ�����������ֵ������ɫ��ʱ�����ߵ���0ʱ������ʾ������ɫ��һ����Ҫ
}__attribute__((packed)) BITMAPINFOHEADER;

typedef struct
{
	unsigned short blue;
	unsigned short green;
	unsigned short red;
	unsigned short reserved;
}__attribute__((packed)) PIXEL;//��ɫģʽRGB

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

	//����ʾ�豸
	fbfd = open(fb_device, O_RDWR);
	if (!fbfd)
	{
		printf("Error: cannot open framebuffer device.\n");
		exit(1);
	}

	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo))
	{
		printf("Error��reading fixed information.\n");
		exit(2);
	}

	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo))
	{
		printf("Error: reading variable information.\n");
		exit(3);
	}
	
	printf("vinfo.xres=%d\n",vinfo.xres);   //��Ļ��
	printf("vinfo.yres=%d\n",vinfo.yres);   //��Ļ��
	printf("vinfo.xoffset=%d\n",vinfo.xoffset); //��ǰ��Ļ��xƫ��
  printf("vinfo.yoffset=%d\n",vinfo.yoffset); //��ǰ��Ļ��yƫ�ƣ�android��˫��������������壬һ֡���ݰ����������������ݣ�����������������
  
  printf("vinfo.xres_virtual=%d\n",vinfo.xres_virtual);   //һ֡���ݵ���Ļ��
	printf("vinfo.yres_virtual=%d\n",vinfo.yres_virtual);   //һ֡���ݵ���Ļ�ߣ�һ�����2*yres��˫����ʱ��
  
  printf("vinfo.bits_per_pixel=%d\n",vinfo.bits_per_pixel);
  
  printf("vinfo.red.offset=%d\n",vinfo.red.offset);   //r��bits��ƫ����
  printf("vinfo.red.length=%d\n",vinfo.red.length);   //r��bits���ж��ٸ�bit
  printf("vinfo.red.msb_right=%d\n",vinfo.red.msb_right); //0 ����Big endian 
  printf("vinfo.green.offset=%d\n",vinfo.green.offset);
  printf("vinfo.green.length=%d\n",vinfo.green.length);
  printf("vinfo.green.msb_right=%d\n",vinfo.green.msb_right);
  printf("vinfo.blue.offset=%d\n",vinfo.blue.offset);
  printf("vinfo.blue.length=%d\n",vinfo.blue.length);
  printf("vinfo.blue.msb_right=%d\n",vinfo.blue.msb_right);
  
  
  printf("finfo.type=%d\n",finfo.type); //�����Android�豸�� fb0 ��Ӧ���� type == 0 �ģ�type Ϊ0 ��˼˵ Framebuffer ��ͷ�����ÿ�����ص�� ARGB ��Ϣ��
  
  //��ʵ���ǵ� Framebuffer ��ͷ�����һ�����ݲ���һ���պþ������ǵ���Ļ�ֱ��ʴ�С�����ĺ�������ֵ�п��ܱ���Ļ�ĺ�������ֵ�࣡�����㷢����س�����ͼƬ��Ļ���кڱߣ���ԭ����������ˡ�
  printf("finfo.line_length=%d\n",finfo.line_length);  //Framebuffer��ͷ��ͼ�����ݵ�һ�У���Ӧ���� ��Ļ����ֱ��� * 4����Ӧ���� line_length (���ĵ�λ�������ص㣬�����ֽ�) 
  
	xres = vinfo.xres;
	yres = vinfo.yres;
	bits_per_pixel = vinfo.bits_per_pixel;
	yoffset = vinfo.yoffset;
	line_length = finfo.line_length;
	
	
	//������Ļ���ܴ�С���ֽڣ�
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

	//����ӳ��
	fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
	if ((int)fbp == -1)
	{
		printf("Error: failed to map framebuffer device to memory.\n");
		exit(4);
	}
	memset(fbp,0,screensize); 
	
	printf("sizeof file header=%d\n", sizeof(BITMAPFILEHEADER));

	printf("into show_bmp function\n");

	//��ʾͼ��
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
    
	//ɾ������ӳ��
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
	
	//����Ƿ���bmpͼ��
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
	
	
	//��ת��������
	fseek(fp, FileHead.cfoffBits, SEEK_SET);
	//ÿ���ֽ���
	BytesPerLine = (InfoHead.ciWidth * InfoHead.ciBitCount + 31) / 32 * 4;

	line_x = line_y = 0;
	//��framebuffer��дBMPͼƬ
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
		

		//��ʾÿһ������
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


