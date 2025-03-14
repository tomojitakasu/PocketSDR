/*------------------------------------------------------------------------------
* convbin.c : convert receiver binary log file to rinex obs/nav, sbas messages
*
*          Copyright (C) 2007-2021 by T.TAKASU, All rights reserved.
*
* options : -DWIN32 use windows file path separator
*
* version : $Revision: 1.1 $ $Date: 2008/07/17 22:13:04 $
* history : 2008/06/22 1.0 new
*           2009/06/17 1.1 support glonass
*           2009/12/19 1.2 fix bug on disable of glonass
*                          fix bug on inproper header for rtcm2 and rtcm3
*           2010/07/18 1.3 add option -v, -t, -h, -x
*           2011/01/15 1.4 add option -ro, -hc, -hm, -hn, -ht, -ho, -hr, -ha,
*                            -hp, -hd, -y, -c, -q 
*                          support gw10 and javad receiver, galileo, qzss
*                          support rinex file name convention
*           2012/10/22 1.5 add option -scan, -oi, -ot, -ol
*                          change default rinex version to 2.11
*                          fix bug on default output directory (/ -> .)
*                          support galileo nav (LNAV) output
*                          support compass
*           2012/11/19 1.6 fix bug on setting code mask in rinex options
*           2013/02/18 1.7 support binex
*           2013/05/19 1.8 support auto format for file path with wild-card
*           2014/02/08 1.9 add option -span -trace -mask
*           2014/08/26 1.10 add Trimble RT17 support
*           2014/12/26 1.11 add option -nomask
*           2016/01/23 1.12 enable septentrio
*           2016/05/25 1.13 fix bug on initializing output file paths in
*                           convbin()
*           2016/06/09 1.14 fix bug on output file with -v 3.02
*           2016/07/01 1.15 support log format CMR/CMR+
*           2016/07/31 1.16 add option -halfc
*           2017/05/26 1.17 add input format tersus
*           2017/06/06 1.18 fix bug on output beidou and irnss nav files
*                           add option -tt
*           2018/10/10 1.19 default options are changed.
*                             scan input file: off - on
*                             number of freq: 2 -> 3
*                           add option -noscan
*           2020/11/30 1.20 include NavIC in default systems
*                           force option -scan
*                           delete option -noscan
*                           surppress warnings
*           2021/01/07 1.21 add option -ver
*                           update help text
*                           add patch level of program name in RINEX header
*
*           2025/02/21 1.22 Pocket SDR branch for RTCM3 conversion
*                           - default RINEX version -> 3.05
*                           - add options -xd and -xs
*                           - delete options -od and -os
*                           - support RTCM3 MSM signal ID extenstions
*                           - disable sbas log output as default
*                           - disable globbing of file path for Windows
*-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "rtklib.h"

#define PRGNAME   "CONVBIN"
#define TRACEFILE "convbin.trace"
#define NOUTFILE        9       /* number of output files */

#ifdef WIN32
int _CRT_glob = 0; /* disable globbing */
#endif

/* help text -----------------------------------------------------------------*/
static const char *help[]={
"",
" Synopsis",
"",
" convbin [option ...] file", 
"",
" Description",
"",
" Convert RTCM, receiver raw data log and RINEX file to RINEX and SBAS",
" message files. SBAS message files complie with RTKLIB SBAS message",
" format. It supports the following messages or files.",
"",
" VENDOR/FORMAT/RECEIVER: MESSAGES, MESSAGE IDS or FILES",
" -----------------------------------------------------------------------------",
" RTCM 2                : Type 1, 3, 9, 14, 16, 17, 18, 19, 22",
" RTCM 3                : Type 1002, 1004, 1005, 1006, 1007, 1008, 1010, 1012,",
"                         1019, 1020, 1029, 1033, 1041, 1044, 1045, 1046, 1042,",
"                         1074, 1075, 1076, 1077, 1084, 1085, 1086, 1087, 1094,",
"                         1095, 1096, 1097, 1104, 1105, 1106, 1107, 1114, 1115,",
"                         1116, 1117, 1124, 1125, 1126, 1127, 1230",
" NovAtel OEM4/V/5/6/7/ : RANGECMPB, RANGEB, RAWEPHEMB, IONUTCB, RAWWAASFRAMEB,",
"         OEMStar         RAWSBASFRAMEB, GLOEPHEMERISB, GALEPHEMERISB,",
"                         GALIONB, GALCLOCKB, QZSSRAWEPHEMB, QZSSRAWSUBFRAMEB,",
"                         BDSEPHEMERISB, NAVICEPHEMERISB",
" NovAtel OEM3          : RGEB, REGD, REPB, FRMB, IONB, UTCB",
" u-blox  4T/5T/6T/7T/  : UBX-RXM-RAW, UBX-RXM-SFRB, UBX-RXM-RAWX,",
"         M8T/M8P/F9      UBS-RXM-SFRBX",
" NovAtel Superstar II  : ID#20, ID#21, ID#22, ID#23, ID#67",
" Hemisphere            : BIN65, BIN66, BIN76, BIN80, BIN94, BIN95, BIN96",
" SkyTraq S1315F        : 0xDC, 0xDD, 0xE0, 0xE1, 0x5C, 0xE2, 0xE3, 0xE5",
" JAVAD GREIS           : [RD], [SI], [NN], [GE], [NE], [EN], [WE], [QE], [CN],",
"                         [IE], [UO], [IO], [GD], [QD], [gd], [qd], [LD], [lD],",
"                         [TC], [R*], [r*], [*R], [*r], [P*], [p*], [*P], [*p],",
"                         [D*], [*d], [E*], [*E], [F*]",
" NVS NV08C BINR        : 0xF5, 0x4A, 0x4B, 0xF7, 0xE5",
" BINEX                 : 0x00, 0x01-01, 0x01-02, 0x01-03, 0x01-04, 0x01-05,",
"                         0x01-06, 0x01-07, 0x01-14, 0x7F-05",
"                         (big-endian, regular CRC, forward record (sync=0xE2))",
" Trimble RT17          : 0x55-1, 0x55-3, 0x57-0",
" Septentrio SBF        : MEASEPOCH, GPSRAWCA, GLORAWCA, GALRAWFNAV,",
"                         GALRAWINAV, GEORAWL1, BDSRAW, QZSRAWL1CA, NAVICRAW",
" RINEX                 : OBS, NAV, GNAV, HNAV, LNAV, QNAV, CNAV, INAV",
"",
" Options [default]",
"",
"     file         Input receiver log file path (wild-cards (*) can be included)",
"     -ts y/m/d h:m:s  Start time [all]",
"     -te y/m/d h:m:s  End time [all]",
"     -tr y/m/d h:m:s  Approximated log start time for RTCM [see below]",
"     -ti tint     Observation data epoch interval (s) [all]",
"     -tt ttol     Observation data epoch tolerance (s) [0.005]",
"     -span span   Time span (h) [all]",
"     -r format    Receiver log format",
"                  rtcm2= RTCM 2",
"                  rtcm3= RTCM 3",
"                  nov  = NovAtel OEM4/V/6/7/OEMStar",
"                  oem3 = NovAtel OEM3",
"                  ubx  = ublox 4T/5T/6T/7T/M8T/M8P/F9",
"                  ss2  = NovAtel Superstar II",
"                  hemis= Hemisphere",
"                  stq  = SkyTraq S1315F",
"                  javad= JAVAD GREIS",
"                  nvs  = NVS NV08C BINR",
"                  binex= BINEX",
"                  rt17 = Trimble RT17",
"                  sbf  = Septentrio SBF",
"                  rinex= RINEX",
"     -ro opt      Receiver options",
"     -f freq      Number of signal frequencies [5]",
"     -hc comment  RINEX header: comment line",
"     -hm marker   RINEX header: marker name",
"     -hn markno   RINEX header: marker number",
"     -ht marktype RINEX header: marker type",
"     -ho observ   RINEX header: observer name and agency separated by /",
"     -hr rec      RINEX header: receiver number, type and version separated by /",
"     -ha ant      RINEX header: antenna number and type separated by /",
"     -hp pos      RINEX header: approx position x/y/z separated by /",
"     -hd delta    RINEX header: antenna delta h/e/n separated by /",
"     -v ver       RINEX version [3.05]",
"     -xd          Exclude Doppler frequency in RINEX OBS file [off]",
"     -xs          Exclude SNR in RINEX OBS file [off]",
"     -oi          Include iono correction in RINEX NAV header [off]",
"     -ot          Include time correction in RINEX NAV header [off]",
"     -ol          Include leap seconds in RINEX NAV header [off]",
"     -halfc       Half-cycle ambiguity correction [off]",
"     -mask   [sig[,...]] Signal mask(s) (sig={G|R|E|J|S|C|I}L{1C|1P|1W|...})",
"     -nomask [sig[,...]] Signal no mask(s) (same as above)",
"     -x sat[,...] Excluded satellite(s)",
"     -y sys[,...] Excluded system(s)",
"                  (G:GPS,R:GLONASS,E:Galileo,J:QZSS,S:SBAS,C:BDS,I:NavIC)",
"     -d dir       Output directory path [same as input directory]",
"     -c staid     Used RINEX file name convention with station ID staid [off]",
"     -o ofile     Output OBS file path",
"     -n nfile     Output GPS or mixed NAV file path",
"     -g gfile     Output GLONASS NAV file path",
"     -h hfile     Output SBAS NAV file path",
"     -q qfile     Output QZSS NAV file path  (RINEX ver.3)",
"     -l lfile     Output Galileo NAV file path",
"     -b cfile     Output BDS NAV file path   (RINEX ver.3)",
"     -i ifile     Output NavIC NAV file path (RINEX ver.3)",
"     -s sfile     Output SBAS message file path",
"     -trace level Output debug trace level [off]",
"     -ver         Print version",
"",
" If the input file path contains wild-card(s) (*), multiple files matching to",
" the path are selected as inputs in dictionary order. In this case, the path",
" should be quoted to avoid expansion by command shell.",
" If no output file path specified, default output file paths, <file>.obs,",
" <file>.nav (for RINEX ver.3), <file>.nav, <file>.gnav, <file>.hnav, <file>.lnav",
" (for RINEX ver.2) and <file>.sbs (<file>: input file path without extension),",
" are used.",
" To resolve week ambiguity in RTCM file, use -tr option to specify the ",
" approximated log start time. Without -tr option, the program obtains the time",
" from the time-tag file (if it exists) or the last modified time of the input",
" file instead.",
"",
" If receiver type is not specified, type is recognized by the input",
" file extension as follows.",
"     *.rtcm2       RTCM 2",
"     *.rtcm3       RTCM 3",
"     *.gps         NovAtel OEM4/V/6/7,OEMStar",
"     *.ubx         u-blox 4T/5T/6T/7T/M8T/M8P/F9",
"     *.log         NovAtel Superstar II",
"     *.bin         Hemisphere",
"     *.stq         SkyTraq S1315F",
"     *.jps         JAVAD GREIS",
"     *.bnx,*binex  BINEX",
"     *.rt17        Trimble RT17",
"     *.sbf         Septentrio SBF",
"     *.obs,*.*o    RINEX OBS",
"     *.rnx         RINEX OBS"
"     *.nav,*.*n    RINEX NAV",
};
/* print help ----------------------------------------------------------------*/
static void printhelp(void)
{
    int i;
    for (i=0;i<(int)(sizeof(help)/sizeof(*help));i++) fprintf(stderr,"%s\n",help[i]);
    exit(0);
}
/* print version -------------------------------------------------------------*/
static void printver(void)
{
    fprintf(stderr,"%s ver.%s %s\n",PRGNAME,VER_RTKLIB,PATCH_LEVEL);
    exit(0);
}
/* show message --------------------------------------------------------------*/
extern int showmsg(const char *format, ...)
{
    va_list arg;
    va_start(arg,format); vfprintf(stderr,format,arg); va_end(arg);
    fprintf(stderr,*format?"\r":"\n");
    return 0;
}
/* copy string within max length ---------------------------------------------*/
static void strcpy_n(char *dst, const char *src, int max_len)
{
    sprintf(dst,"%.*s",max_len,src);
}
/* convert main --------------------------------------------------------------*/
static int convbin(int format, rnxopt_t *opt, const char *ifile, char **file,
                   char *dir)
{
    int i,def;
    static char work[256],ofile_[NOUTFILE][1024]={"","","","","","","","",""};
    char ifile_[1024],*ofile[NOUTFILE],*p;
    char *extnav=(opt->rnxver<=299||opt->navsys==SYS_GPS)?"N":"P";
    
    /* replace wild-card (*) in input file by 0 */
    strcpy_n(ifile_,ifile,1023);
    for (p=ifile_;*p;p++) if (*p=='*') *p='0';

    def=!file[0]&&!file[1]&&!file[2]&&!file[3]&&!file[4]&&!file[5]&&!file[6]&&
        !file[7]&&!file[8];
    
    for (i=0;i<NOUTFILE;i++) ofile[i]=ofile_[i];
    
    if (file[0]) { /* OBS */
        strcpy_n(ofile[0],file[0],1023);
    }
    else if (*opt->staid) {
        strcpy_n(ofile[0],"%r%n0.%yO",1023);
    }
    else if (def) {
        strcpy_n(ofile[0],ifile_,1019);
        if ((p=strrchr(ofile[0],'.'))) strcpy(p,".obs");
        else strcat(ofile[0],".obs");
    }
    if (file[1]) { /* GPS or Mixed NAV */
        strcpy_n(ofile[1],file[1],1023);
    }
    else if (*opt->staid) {
        strcpy_n(ofile[1],"%r%n0.%y",1022);
        strcat(ofile[1],extnav);
    }
    else if (def) {
        strcpy_n(ofile[1],ifile_,1019);
        if ((p=strrchr(ofile[1],'.'))) strcpy(p,".nav");
        else strcat(ofile[1],".nav");
    }
    if (file[2]) { /* GLONASS NAV */
        strcpy_n(ofile[2],file[2],1023);
    }
    else if (opt->rnxver<=299&&*opt->staid) {
        strcpy_n(ofile[2],"%r%n0.%yG",1023);
    }
    else if (opt->rnxver<=299&&def) {
        strcpy_n(ofile[2],ifile_,1018);
        if ((p=strrchr(ofile[2],'.'))) strcpy(p,".gnav");
        else strcat(ofile[2],".gnav");
    }
    if (file[3]) { /* GEO NAV */
        strcpy_n(ofile[3],file[3],1023);
    }
    else if (opt->rnxver<=299&&*opt->staid) {
        strcpy_n(ofile[3],"%r%n0.%yH",1023);
    }
    else if (opt->rnxver<=299&&def) {
        strcpy_n(ofile[3],ifile_,1018);
        if ((p=strrchr(ofile[3],'.'))) strcpy(p,".hnav");
        else strcat(ofile[3],".hnav");
    }
    if (opt->rnxver>=302&&file[4]) { /* QZSS NAV */
        strcpy_n(ofile[4],file[4],1023);
    }
    if (opt->rnxver>=212&&file[5]) { /* Galileo NAV */
        strcpy_n(ofile[5],file[5],1023);
    }
    else if (opt->rnxver>=212&&opt->rnxver<=299&&*opt->staid) {
        strcpy_n(ofile[5],"%r%n0.%yL",1023);
    }
    else if (opt->rnxver>=212&&opt->rnxver<=299&&def) {
        strcpy_n(ofile[5],ifile_,1018);
        if ((p=strrchr(ofile[5],'.'))) strcpy(p,".lnav");
        else strcat(ofile[5],".lnav");
    }
    if (opt->rnxver>=301&&file[6]) { /* BDS NAV */
        strcpy_n(ofile[6],file[6],1023);
    }
    if (opt->rnxver>=303&&file[7]) { /* NavIC NAV */
        strcpy_n(ofile[7],file[7],1023);
    }
    if (file[8]) { /* SBAS message */
        strcpy_n(ofile[8],file[8],1023);
    }
#if 0
    else if (*opt->staid) {
        strcpy(ofile[8],"%r%n0_%y.sbs");
    }
    else if (def) {
        strcpy_n(ofile[8],ifile_,1019);
        if ((p=strrchr(ofile[8],'.'))) strcpy(p,".sbs");
        else strcat(ofile[8],".sbs");
    }
#endif
    for (i=0;i<NOUTFILE;i++) {
        if (!*dir||!*ofile[i]) continue;
        if ((p=strrchr(ofile[i],FILEPATHSEP))) strcpy_n(work,p+1,255);
        else strcpy_n(work,ofile[i],255);
        sprintf(ofile[i],"%.767s%c%.255s",dir,FILEPATHSEP,work);
    }
    fprintf(stderr,"input file  : %s (%s)\n",ifile,formatstrs[format]);
    
    if (*ofile[0]) fprintf(stderr,"->rinex obs : %s\n",ofile[0]);
    if (*ofile[1]) fprintf(stderr,"->rinex nav : %s\n",ofile[1]);
    if (*ofile[2]) fprintf(stderr,"->rinex gnav: %s\n",ofile[2]);
    if (*ofile[3]) fprintf(stderr,"->rinex hnav: %s\n",ofile[3]);
    if (*ofile[4]) fprintf(stderr,"->rinex qnav: %s\n",ofile[4]);
    if (*ofile[5]) fprintf(stderr,"->rinex lnav: %s\n",ofile[5]);
    if (*ofile[6]) fprintf(stderr,"->rinex cnav: %s\n",ofile[6]);
    if (*ofile[7]) fprintf(stderr,"->rinex inav: %s\n",ofile[7]);
    if (*ofile[8]) fprintf(stderr,"->sbas log  : %s\n",ofile[8]);
    
    if (!convrnx(format,opt,ifile,ofile)) {
        fprintf(stderr,"\n");
        return -1;
    }
    fprintf(stderr,"\n");
    return 0;
}
/* set signal mask -----------------------------------------------------------*/
static void setmask(const char *argv, rnxopt_t *opt, int mask)
{
    char buff[1024],*p;
    int i,code;
    
    strcpy(buff,argv);
    for (p=strtok(buff,",");p;p=strtok(NULL,",")) {
        if (strlen(p)<4||p[1]!='L') continue;
        if      (p[0]=='G') i=0;
        else if (p[0]=='R') i=1;
        else if (p[0]=='E') i=2;
        else if (p[0]=='J') i=3;
        else if (p[0]=='S') i=4;
        else if (p[0]=='C') i=5;
        else if (p[0]=='I') i=6;
        else continue;
        if ((code=obs2code(p+2))) {
            opt->mask[i][code-1]=mask?'1':'0';
        }
    }
}
/* get start time of input file -----------------------------------------------*/
static int get_filetime(const char *file, gtime_t *time)
{
    FILE *fp;
    struct stat st;
    struct tm *tm;
    uint32_t time_time;
    uint8_t buff[64];
    double ep[6];
    char path[1024],*paths[1],path_tag[1024];

    paths[0]=path;
    
    if (!expath(file,paths,1)) return 0;
    
    /* get start time of time-tag file */
    sprintf(path_tag,"%.1019s.tag",path);
    if ((fp=fopen(path_tag,"rb"))) {
        if (fread(buff,64,1,fp)==1&&!strncmp((char *)buff,"TIMETAG",7)&&
            fread(&time_time,4,1,fp)==1) {
            time->time=time_time; 
            time->sec=0.0;
            fclose(fp);
            return 1;
		}
        fclose(fp);
	}
    /* get modified time of input file */
    if (!stat(path,&st)&&(tm=gmtime(&st.st_mtime))) {
        ep[0]=tm->tm_year+1900;
        ep[1]=tm->tm_mon+1;
        ep[2]=tm->tm_mday;
        ep[3]=tm->tm_hour;
        ep[4]=tm->tm_min;
        ep[5]=tm->tm_sec;
        *time=utc2gpst(epoch2time(ep));
        return 1;
    }
    return 0;
}
/* parse command line options ------------------------------------------------*/
static int cmdopts(int argc, char **argv, rnxopt_t *opt, char **ifile,
                   char **ofile, char **dir, int *trace)
{
    double eps[]={1980,1,1,0,0,0},epe[]={2037,12,31,0,0,0};
    double epr[]={2010,1,1,0,0,0},span=0.0;
    int i,j,k,sat,nf=5,nc=2,format=-1;
    char *p,*fmt="",*paths[1],path[1024],buff[256];
    
    opt->rnxver=305;
    opt->obstype=OBSTYPE_PR|OBSTYPE_CP|OBSTYPE_DOP|OBSTYPE_SNR;
    opt->navsys=SYS_GPS|SYS_GLO|SYS_GAL|SYS_QZS|SYS_SBS|SYS_CMP|SYS_IRN;
    
    for (i=0;i<6;i++) for (j=0;j<MAXCODE;j++) {
        opt->mask[i][j]='1';
    }
    for (i=1;i<argc;i++) {
        if (!strcmp(argv[i],"-ts")&&i+2<argc) {
            sscanf(argv[++i],"%lf/%lf/%lf",eps,eps+1,eps+2);
            sscanf(argv[++i],"%lf:%lf:%lf",eps+3,eps+4,eps+5);
            opt->ts=epoch2time(eps);
        }
        else if (!strcmp(argv[i],"-te")&&i+2<argc) {
            sscanf(argv[++i],"%lf/%lf/%lf",epe,epe+1,epe+2);
            sscanf(argv[++i],"%lf:%lf:%lf",epe+3,epe+4,epe+5);
            opt->te=epoch2time(epe);
        }
        else if (!strcmp(argv[i],"-tr")&&i+2<argc) {
            sscanf(argv[++i],"%lf/%lf/%lf",epr,epr+1,epr+2);
            sscanf(argv[++i],"%lf:%lf:%lf",epr+3,epr+4,epr+5);
            opt->trtcm=epoch2time(epr);
        }
        else if (!strcmp(argv[i],"-ti")&&i+1<argc) {
            opt->tint=atof(argv[++i]);
        }
        else if (!strcmp(argv[i],"-tt")&&i+1<argc) {
            opt->ttol=atof(argv[++i]);
        }
        else if (!strcmp(argv[i],"-span")&&i+1<argc) {
            span=atof(argv[++i]);
        }
        else if (!strcmp(argv[i],"-r" )&&i+1<argc) {
            fmt=argv[++i];
        }
        else if (!strcmp(argv[i],"-ro")&&i+1<argc) {
            strcpy_n(opt->rcvopt,argv[++i],255);
        }
        else if (!strcmp(argv[i],"-f" )&&i+1<argc) {
            nf=atoi(argv[++i]);
        }
        else if (!strcmp(argv[i],"-hc")&&i+1<argc) {
            if (nc<MAXCOMMENT) strcpy_n(opt->comment[nc++],argv[++i],63);
        }
        else if (!strcmp(argv[i],"-hm")&&i+1<argc) {
            strcpy_n(opt->marker,argv[++i],63);
        }
        else if (!strcmp(argv[i],"-hn")&&i+1<argc) {
            strcpy_n(opt->markerno,argv[++i],31);
        }
        else if (!strcmp(argv[i],"-ht")&&i+1<argc) {
            strcpy_n(opt->markertype,argv[++i],31);
        }
        else if (!strcmp(argv[i],"-ho")&&i+1<argc) {
            strcpy_n(buff,argv[++i],255);
            for (j=0,p=strtok(buff,"/");j<2&&p;j++,p=strtok(NULL,"/")) {
                strcpy_n(opt->name[j],p,31);
            }
        }
        else if (!strcmp(argv[i],"-hr")&&i+1<argc) {
            strcpy_n(buff,argv[++i],255);
            for (j=0,p=strtok(buff,"/");j<3&&p;j++,p=strtok(NULL,"/")) {
                strcpy_n(opt->rec[j],p,31);
            }
        }
        else if (!strcmp(argv[i],"-ha")&&i+1<argc) {
            strcpy_n(buff,argv[++i],255);
            for (j=0,p=strtok(buff,"/");j<3&&p;j++,p=strtok(NULL,"/")) {
                strcpy_n(opt->ant[j],p,31);
            }
        }
        else if (!strcmp(argv[i],"-hp")&&i+1<argc) {
            strcpy_n(buff,argv[++i],255);
            for (j=0,p=strtok(buff,"/");j<3&&p;j++,p=strtok(NULL,"/")) {
                opt->apppos[j]=atof(p);
            }
        }
        else if (!strcmp(argv[i],"-hd")&&i+1<argc) {
            strcpy_n(buff,argv[++i],255);
            for (j=0,p=strtok(buff,"/");j<3&&p;j++,p=strtok(NULL,"/")) {
                opt->antdel[j]=atof(p);
            }
        }
        else if (!strcmp(argv[i],"-v" )&&i+1<argc) {
            opt->rnxver=(int)(atof(argv[++i])*100.0);
        }
        else if (!strcmp(argv[i],"-xd")) {
            opt->obstype&=~OBSTYPE_DOP;
        }
        else if (!strcmp(argv[i],"-xs")) {
            opt->obstype&=~OBSTYPE_SNR;
        }
        else if (!strcmp(argv[i],"-oi")) {
            opt->outiono=1;
        }
        else if (!strcmp(argv[i],"-ot")) {
            opt->outtime=1;
        }
        else if (!strcmp(argv[i],"-ol")) {
            opt->outleaps=1;
        }
        else if (!strcmp(argv[i],"-scan")) {
            /* obsolute, ignored */ ;
        }
        else if (!strcmp(argv[i],"-halfc")) {
            opt->halfcyc=1;
        }
        else if (!strcmp(argv[i],"-mask")&&i+1<argc) {
            for (j=0;j<7;j++) for (k=0;k<MAXCODE;k++) opt->mask[j][k]='0';
            strcpy_n(buff,argv[++i],255);
            for (p=strtok(buff,",");p;p=strtok(NULL,",")) {
                setmask(p,opt,1);
            }
        }
        else if (!strcmp(argv[i],"-nomask")&&i+1<argc) {
            strcpy_n(buff,argv[++i],255);
            for (p=strtok(buff,",");p;p=strtok(NULL,",")) {
                setmask(p,opt,0);
            }
        }
        else if (!strcmp(argv[i],"-x" )&&i+1<argc) {
            strcpy_n(buff,argv[++i],255);
            for (p=strtok(buff,",");p;p=strtok(NULL,",")) {
                if ((sat=satid2no(p))) opt->exsats[sat-1]=1;
            }
        }
        else if (!strcmp(argv[i],"-y" )&&i+1<argc) {
            strcpy_n(buff,argv[++i],255);
            for (p=strtok(buff,",");p;p=strtok(NULL,",")) {
                if      (!strcmp(p,"G")) opt->navsys&=~SYS_GPS;
                else if (!strcmp(p,"R")) opt->navsys&=~SYS_GLO;
                else if (!strcmp(p,"E")) opt->navsys&=~SYS_GAL;
                else if (!strcmp(p,"J")) opt->navsys&=~SYS_QZS;
                else if (!strcmp(p,"S")) opt->navsys&=~SYS_SBS;
                else if (!strcmp(p,"C")) opt->navsys&=~SYS_CMP;
                else if (!strcmp(p,"I")) opt->navsys&=~SYS_IRN;
            }
        }
        else if (!strcmp(argv[i],"-d" )&&i+1<argc) {
            *dir=argv[++i];
        }
        else if (!strcmp(argv[i],"-c" )&&i+1<argc) {
            strcpy_n(opt->staid,argv[++i],31);
        }
        else if (!strcmp(argv[i],"-o" )&&i+1<argc) ofile[0]=argv[++i];
        else if (!strcmp(argv[i],"-n" )&&i+1<argc) ofile[1]=argv[++i];
        else if (!strcmp(argv[i],"-g" )&&i+1<argc) ofile[2]=argv[++i];
        else if (!strcmp(argv[i],"-h" )&&i+1<argc) ofile[3]=argv[++i];
        else if (!strcmp(argv[i],"-q" )&&i+1<argc) ofile[4]=argv[++i];
        else if (!strcmp(argv[i],"-l" )&&i+1<argc) ofile[5]=argv[++i];
        else if (!strcmp(argv[i],"-b" )&&i+1<argc) ofile[6]=argv[++i];
        else if (!strcmp(argv[i],"-i" )&&i+1<argc) ofile[7]=argv[++i];
        else if (!strcmp(argv[i],"-s" )&&i+1<argc) ofile[8]=argv[++i];
        else if (!strcmp(argv[i],"-trace" )&&i+1<argc) {
            *trace=atoi(argv[++i]);
        }
        else if (!strcmp(argv[i],"-ver")) printver();
        else if (!strncmp(argv[i],"-",1)) printhelp();
        
        else *ifile=argv[i];
    }
    if (span>0.0&&opt->ts.time) {
        opt->te=timeadd(opt->ts,span*3600.0-1e-3);
    }
    if (nf>=1) opt->freqtype|=FREQTYPE_L1;
    if (nf>=2) opt->freqtype|=FREQTYPE_L2;
    if (nf>=3) opt->freqtype|=FREQTYPE_L3;
    if (nf>=4) opt->freqtype|=FREQTYPE_L4;
    if (nf>=5) opt->freqtype|=FREQTYPE_L5;
    
    if (!opt->trtcm.time) {
        get_filetime(*ifile,&opt->trtcm);
    }
    if (*fmt) {
        if      (!strcmp(fmt,"rtcm2")) format=STRFMT_RTCM2;
        else if (!strcmp(fmt,"rtcm3")) format=STRFMT_RTCM3;
        else if (!strcmp(fmt,"nov"  )) format=STRFMT_OEM4;
        else if (!strcmp(fmt,"oem3" )) format=STRFMT_OEM3;
        else if (!strcmp(fmt,"ubx"  )) format=STRFMT_UBX;
        else if (!strcmp(fmt,"ss2"  )) format=STRFMT_SS2;
        else if (!strcmp(fmt,"hemis")) format=STRFMT_CRES;
        else if (!strcmp(fmt,"stq"  )) format=STRFMT_STQ;
        else if (!strcmp(fmt,"javad")) format=STRFMT_JAVAD;
        else if (!strcmp(fmt,"nvs"  )) format=STRFMT_NVS;
        else if (!strcmp(fmt,"binex")) format=STRFMT_BINEX;
        else if (!strcmp(fmt,"rt17" )) format=STRFMT_RT17;
        else if (!strcmp(fmt,"sbf"  )) format=STRFMT_SEPT;
        else if (!strcmp(fmt,"rinex")) format=STRFMT_RINEX;
    }
    else {
        paths[0]=path;
        if (!expath(*ifile,paths,1)||!(p=strrchr(path,'.'))) return -1;
        if      (!strcmp(p,".rtcm2"))  format=STRFMT_RTCM2;
        else if (!strcmp(p,".rtcm3"))  format=STRFMT_RTCM3;
        else if (!strcmp(p,".gps"  ))  format=STRFMT_OEM4;
        else if (!strcmp(p,".ubx"  ))  format=STRFMT_UBX;
        else if (!strcmp(p,".log"  ))  format=STRFMT_SS2;
        else if (!strcmp(p,".bin"  ))  format=STRFMT_CRES;
        else if (!strcmp(p,".stq"  ))  format=STRFMT_STQ;
        else if (!strcmp(p,".jps"  ))  format=STRFMT_JAVAD;
        else if (!strcmp(p,".bnx"  ))  format=STRFMT_BINEX;
        else if (!strcmp(p,".binex"))  format=STRFMT_BINEX;
        else if (!strcmp(p,".rt17" ))  format=STRFMT_RT17;
        else if (!strcmp(p,".sbf"  ))  format=STRFMT_SEPT;
        else if (!strcmp(p,".obs"  ))  format=STRFMT_RINEX;
        else if (!strcmp(p+3,"o"   ))  format=STRFMT_RINEX;
        else if (!strcmp(p+3,"O"   ))  format=STRFMT_RINEX;
        else if (!strcmp(p,".rnx"  ))  format=STRFMT_RINEX;
        else if (!strcmp(p,".nav"  ))  format=STRFMT_RINEX;
        else if (!strcmp(p+3,"n"   ))  format=STRFMT_RINEX;
        else if (!strcmp(p+3,"N"   ))  format=STRFMT_RINEX;
    }
    return format;
}
/* main ----------------------------------------------------------------------*/
int main(int argc, char **argv)
{
    rnxopt_t opt={{0}};
    int format,trace=0,stat;
    char *ifile="",*ofile[NOUTFILE]={0},*dir="";
    
    /* parse command line options */
    format=cmdopts(argc,argv,&opt,&ifile,ofile,&dir,&trace);
    
    if (!*ifile) {
        fprintf(stderr,"no input file\n");
        return -1;
    }
    if (format<0) {
        fprintf(stderr,"input format can not be recognized\n");
        return -1;
    }
    sprintf(opt.prog,"%s %s %s",PRGNAME,VER_RTKLIB,PATCH_LEVEL);
    
    if (trace>0) {
        traceopen(TRACEFILE);
        tracelevel(trace);
    }
    stat=convbin(format,&opt,ifile,ofile,dir);
    
    traceclose();
    
    return stat;
}
