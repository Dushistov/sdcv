#ifndef _m_ctype_h
#define _m_ctype_h

#ifdef  __cplusplus
extern "C" {
#endif

#define my_wc_t ulong

typedef struct unicase_info_st
{
  uint16 toupper;
  uint16 tolower;
  uint16 sort;
} MY_UNICASE_INFO;

extern MY_UNICASE_INFO *my_unicase_default[256];
//extern MY_UNICASE_INFO *my_unicase_turkish[256];


/* wm_wc and wc_mb return codes */
#define MY_CS_ILSEQ     0     /* Wrong by sequence: wb_wc                   */
#define MY_CS_ILUNI     0     /* Cannot encode Unicode to charset: wc_mb    */
#define MY_CS_TOOSMALL  -101  /* Need at least one byte:    wc_mb and mb_wc */
#define MY_CS_TOOSMALL2 -102  /* Need at least two bytes:   wc_mb and mb_wc */
#define MY_CS_TOOSMALL3 -103  /* Need at least three bytes: wc_mb and mb_wc */
/* A helper macros for "need at least n bytes" */
#define MY_CS_TOOSMALLN(n)    (-100-(n))

        /* My charsets_list flags */
#define MY_CS_COMPILED  1      /* compiled-in sets               */
#define MY_CS_CONFIG    2      /* sets that have a *.conf file   */
#define MY_CS_INDEX     4      /* sets listed in the Index file  */
#define MY_CS_LOADED    8      /* sets that are currently loaded */
#define MY_CS_BINSORT   16     /* if binary sort order           */
#define MY_CS_PRIMARY   32     /* if primary collation           */
#define MY_CS_STRNXFRM  64     /* if strnxfrm is used for sort   */
#define MY_CS_UNICODE   128    /* is a charset is full unicode   */
#define MY_CS_READY     256    /* if a charset is initialized    */
#define MY_CS_AVAILABLE 512    /* If either compiled-in or loaded*/
#define MY_CS_CSSORT    1024   /* if case sensitive sort order   */
#define MY_CS_HIDDEN    2048   /* don't display in SHOW          */
#define MY_CHARSET_UNDEFINED 0


/*typedef struct my_uni_idx_st
{
  uint16 from;
  uint16 to;
  uchar  *tab;
} MY_UNI_IDX;

typedef struct
{
  uint beg;
  uint end;
  uint mblen;
} my_match_t;*/

struct charset_info_st;

typedef struct my_collation_handler_st
{
  my_bool (*init)(struct charset_info_st *, void *(*alloc)(uint));
  /* Collation routines */
  int     (*strnncoll)(struct charset_info_st *,
		       const uchar *, uint, const uchar *, uint, my_bool);
  //int     (*strnncollsp)(struct charset_info_st *,
                         //const uchar *, uint, const uchar *, uint,
                         //my_bool diff_if_only_endspace_difference);
  //int     (*strnxfrm)(struct charset_info_st *,
		      //uchar *, uint, const uchar *, uint);
  //uint    (*strnxfrmlen)(struct charset_info_st *, uint); 
  //my_bool (*like_range)(struct charset_info_st *,
			//const char *s, uint s_length,
			//pchar w_prefix, pchar w_one, pchar w_many, 
			//uint res_length,
			//char *min_str, char *max_str,
			//uint *min_len, uint *max_len);
  //int     (*wildcmp)(struct charset_info_st *,
  		     //const char *str,const char *str_end,
                     //const char *wildstr,const char *wildend,
                     //int escape,int w_one, int w_many);

  //int  (*strcasecmp)(struct charset_info_st *, const char *, const char *);
  
  //uint (*instr)(struct charset_info_st *,
                //const char *b, uint b_length,
                //const char *s, uint s_length,
                //my_match_t *match, uint nmatch);
  
  /* Hash calculation */
  //void (*hash_sort)(struct charset_info_st *cs, const uchar *key, uint len,
		    //ulong *nr1, ulong *nr2); 
  //my_bool (*propagate)(struct charset_info_st *cs, const uchar *str, uint len);
} MY_COLLATION_HANDLER;

extern MY_COLLATION_HANDLER my_collation_mb_bin_handler;

typedef struct my_charset_handler_st
{
  my_bool (*init)(struct charset_info_st *, void *(*alloc)(uint));
  /* Multibyte routines */
  //int     (*ismbchar)(struct charset_info_st *, const char *, const char *);
  //int     (*mbcharlen)(struct charset_info_st *, uint);
  //uint    (*numchars)(struct charset_info_st *, const char *b, const char *e);
  //uint    (*charpos)(struct charset_info_st *, const char *b, const char *e, uint pos);
  //uint    (*well_formed_len)(struct charset_info_st *,
                             //const char *b,const char *e,
                             //uint nchars, int *error);
  //uint    (*lengthsp)(struct charset_info_st *, const char *ptr, uint length);
  //uint    (*numcells)(struct charset_info_st *, const char *b, const char *e);
  
  /* Unicode convertion */
  int (*mb_wc)(struct charset_info_st *cs,my_wc_t *wc,
	       const unsigned char *s,const unsigned char *e);
  //int (*wc_mb)(struct charset_info_st *cs,my_wc_t wc,
	       //unsigned char *s,unsigned char *e);
  
  /* CTYPE scanner */
  //int (*ctype)(struct charset_info_st *cs, int *ctype,
  //             const unsigned char *s, const unsigned char *e);
  
  /* Functions for case and sort convertion */
  //void    (*caseup_str)(struct charset_info_st *, char *);
  //void    (*casedn_str)(struct charset_info_st *, char *);
  //uint    (*caseup)(struct charset_info_st *, char *src, uint srclen,
  //                                            char *dst, uint dstlen);
  //uint    (*casedn)(struct charset_info_st *, char *src, uint srclen,
  //                                            char *dst, uint dstlen);
  
  /* Charset dependant snprintf() */
  //int  (*snprintf)(struct charset_info_st *, char *to, uint n, const char *fmt,
		   //...);
  //int  (*long10_to_str)(struct charset_info_st *, char *to, uint n, int radix,
			//long int val);
  //int (*longlong10_to_str)(struct charset_info_st *, char *to, uint n,
			   //int radix, longlong val);
  
  //void (*fill)(struct charset_info_st *, char *to, uint len, int fill);
  
  /* String-to-number convertion routines */
  //long        (*strntol)(struct charset_info_st *, const char *s, uint l,
			 //int base, char **e, int *err);
  //ulong      (*strntoul)(struct charset_info_st *, const char *s, uint l,
			 //int base, char **e, int *err);
  //longlong   (*strntoll)(struct charset_info_st *, const char *s, uint l,
			 //int base, char **e, int *err);
  //ulonglong (*strntoull)(struct charset_info_st *, const char *s, uint l,
			 //int base, char **e, int *err);
  //double      (*strntod)(struct charset_info_st *, char *s, uint l, char **e,
			 //int *err);
  //longlong    (*strtoll10)(struct charset_info_st *cs,
                           //const char *nptr, char **endptr, int *error);
  //ulong        (*scan)(struct charset_info_st *, const char *b, const char *e,
		       //int sq);
} MY_CHARSET_HANDLER;

typedef struct charset_info_st
{
  //uint      number;
  //uint      primary_number;
  //uint      binary_number;
  //uint      state;
  //const char *csname;
  //const char *name;
  //const char *comment;
  const char *tailoring;
  //uchar    *ctype;
  //uchar    *to_lower;
  //uchar    *to_upper;
  uchar    *sort_order;
  uint16   *contractions;
  uint16   **sort_order_big;
  //uint16      *tab_to_uni;
  //MY_UNI_IDX  *tab_from_uni;
  MY_UNICASE_INFO **caseinfo;
  //uchar     *state_map;
  //uchar     *ident_map;
  //uint      strxfrm_multiply;
  //uchar     caseup_multiply;
  //uchar     casedn_multiply;
  //uint      mbminlen;
  //uint      mbmaxlen;
  //uint16    min_sort_char;
  //uint16    max_sort_char; /* For LIKE optimization */
  //uchar     pad_char;
  //my_bool   escape_with_backslash_is_dangerous;
  
  MY_CHARSET_HANDLER *cset;
  MY_COLLATION_HANDLER *coll;
  
} CHARSET_INFO;

extern CHARSET_INFO my_charset_utf8_general_ci;
extern CHARSET_INFO my_charset_utf8_bin;

extern CHARSET_INFO my_charset_utf8_general_uca_ci;
extern CHARSET_INFO my_charset_utf8_icelandic_uca_ci;
extern CHARSET_INFO my_charset_utf8_latvian_uca_ci;
extern CHARSET_INFO my_charset_utf8_romanian_uca_ci;
extern CHARSET_INFO my_charset_utf8_slovenian_uca_ci;
extern CHARSET_INFO my_charset_utf8_polish_uca_ci;
extern CHARSET_INFO my_charset_utf8_estonian_uca_ci;
extern CHARSET_INFO my_charset_utf8_spanish_uca_ci;
extern CHARSET_INFO my_charset_utf8_swedish_uca_ci;
extern CHARSET_INFO my_charset_utf8_turkish_uca_ci;
extern CHARSET_INFO my_charset_utf8_czech_uca_ci;
extern CHARSET_INFO my_charset_utf8_danish_uca_ci;
extern CHARSET_INFO my_charset_utf8_lithuanian_uca_ci;
extern CHARSET_INFO my_charset_utf8_slovak_uca_ci;
extern CHARSET_INFO my_charset_utf8_spanish2_uca_ci;
extern CHARSET_INFO my_charset_utf8_roman_uca_ci;
extern CHARSET_INFO my_charset_utf8_persian_uca_ci;
extern CHARSET_INFO my_charset_utf8_esperanto_uca_ci;
extern CHARSET_INFO my_charset_utf8_hungarian_uca_ci;


/*void my_fill_8bit(CHARSET_INFO *cs, char* to, uint l, int fill);
my_bool  my_like_range_mb(CHARSET_INFO *cs,
			  const char *ptr, uint ptr_length,
			  pbool escape, pbool w_one, pbool w_many,
			  uint res_length,
			  char *min_str, char *max_str,
			  uint *min_length, uint *max_length);
uint my_numchars_mb(CHARSET_INFO *, const char *b, const char *e);
uint my_numcells_mb(CHARSET_INFO *, const char *b, const char *e);
uint my_charpos_mb(CHARSET_INFO *, const char *b, const char *e, uint pos);
uint my_well_formed_len_mb(CHARSET_INFO *, const char *b, const char *e,
                           uint pos, int *error);
uint my_instr_mb(struct charset_info_st *,
                 const char *b, uint b_length,
                 const char *s, uint s_length,
                 my_match_t *match, uint nmatch);
int my_mb_ctype_mb(CHARSET_INFO *,int *, const uchar *,const uchar *);
ulong my_scan_8bit(CHARSET_INFO *cs, const char *b, const char *e, int sq);
int my_snprintf_8bit(struct charset_info_st *, char *to, uint n,
		     const char *fmt, ...);
int  my_long10_to_str_8bit(CHARSET_INFO *, char *to, uint l, int radix,
			   long int val);
int my_longlong10_to_str_8bit(CHARSET_INFO *, char *to, uint l, int radix,
			      longlong val);
extern uint my_lengthsp_8bit(CHARSET_INFO *cs, const char *ptr, uint length);
long        my_strntol_8bit(CHARSET_INFO *, const char *s, uint l, int base,
			    char **e, int *err);
ulong      my_strntoul_8bit(CHARSET_INFO *, const char *s, uint l, int base,
			    char **e, int *err);
longlong   my_strntoll_8bit(CHARSET_INFO *, const char *s, uint l, int base,
			    char **e, int *err);
ulonglong my_strntoull_8bit(CHARSET_INFO *, const char *s, uint l, int base,
			    char **e, int *err);
double      my_strntod_8bit(CHARSET_INFO *, char *s, uint l,char **e,
			    int *err);
longlong my_strtoll10_8bit(CHARSET_INFO *cs,
                           const char *nptr, char **endptr, int *error);

my_bool my_propagate_complex(CHARSET_INFO *cs, const uchar *str, uint len);*/


#ifdef	__cplusplus
}
#endif

#endif /* _m_ctype_h */

