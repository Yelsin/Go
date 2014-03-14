//h263.lib header

//#pragma comment(lib, "nafxcw.lib") 
typedef struct compression_parameters 
{
/* Contains all the parameters that are needed for 
   encoding plus all the status between two encodings */
  int half_pixel_searchwindow; /* size of search window in half pixels
				  if this value is 0, no search is performed
				*/
  int format;			/*  */
  int pels;			/* Only used when format == CPARAM_OTHER */
  int lines;			/* Only used when format == CPARAM_OTHER */
  int inter;			/* TRUE of INTER frame encoded frames,
				   FALSE for INTRA frames */
  int search_method;		/* DEF_EXHAUSTIVE or DEF_LOGARITHMIC */
  int advanced_method;		/* TRUE : Use array to determine 
				          macroblocks in INTER frame
					  mode to be encoded */
  int Q_inter;			/* Quantization factor for INTER frames */
  int Q_intra;			/* Quantization factor for INTRA frames */
  unsigned int *data;		/* source data in qcif format */
  unsigned int *interpolated_lum;	/* intepolated recon luminance part */
  unsigned int *recon;		/* Reconstructed copy of compressed frame */
  int *EncodeThisBlock; 
                                /* Array when advanced_method is used */
} CParam;
typedef struct bits_counted
 {
  int Y;
  int C;
  int vec;
  int CBPY;
  int CBPCM;
  int MODB;
  int CBPB;
  int COD;
  int header;
  int DQUANT;
  int total;
  int no_inter;
  int no_inter4v;
  int no_intra;
/* NB: Remember to change AddBits(), ZeroBits() and AddBitsPicture() 
   when entries are added here */
} Bits;
int CompressFrame(CParam *params, Bits *bits);
int InitH263Encoder(CParam *params);
void InitH263Decoder();
void DeleteParam(CParam *params);
void ExitH263Encoder();
void ExitH263Decoder();
void SkipH263Frames(int frames_to_skip);

char* GetCompressData(int &length);
unsigned char * Decoder(unsigned char*ind,int size,int x,int y);
//unsigned char rgb[240000];//output data
int DecompressFrame(unsigned char *cdata,int size,
					unsigned char *outdata,int outsize,
					int x, int y);
/* Procedure to detect motion, expects param->EncodeThisBlock is set to
   array. 
   Advised values for threshold: mb_threholds = 2; pixel_threshold = 2 */
int FindMotion(CParam *params, int mb_threshold, int pixel_threshold);
static void init_motion_detection(void);
//__inline__ static int Check8x8(unsigned int *orig, 
//			       unsigned int *recon, int pos);
static int Check8x8(unsigned int *orig,unsigned int *recon, int pos);
static int HasMoved(int call_time,  void *real, void *recon, int x, int y);




//Interface
void Init263(int ,int);
void Compress(unsigned char*indata, unsigned char**outdata,int &len);
void Decompress(BYTE*indata,int len,BYTE**outdata);
void Close263();
