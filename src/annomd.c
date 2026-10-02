/* AnnoMD - tiny native Win32 Markdown reader/editor. No runtime, no dependencies. */
#define _WIN32_WINNT 0x0601
#define _RICHEDIT_VER 0x0500
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <richedit.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <wctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#define APP L"AnnoMD"

enum{ID_NEW=100,ID_OPEN,ID_SAVE,ID_SAVEAS,ID_EXIT,ID_CLOSE,ID_SAVEALL,ID_NEXT,ID_PREV,
 ID_UNDO=110,ID_REDO,ID_CUT,ID_COPY,ID_PASTE,ID_SELALL,ID_FIND,ID_FINDNEXT,ID_REPLACE,ID_DELETE,
 ID_H1=130,ID_H2,ID_H3,ID_H4,ID_H5,ID_H6,
 ID_NORMAL=140,ID_QUOTE,ID_BULLET,ID_NUMBER,ID_TASK,ID_INDENT,ID_OUTDENT,ID_HR,
 ID_BOLD=160,ID_ITALIC,ID_STRIKE,ID_CODE,ID_CODEBLK,ID_LINK,ID_IMAGE,ID_FONTMORE,
 ID_VEDIT=170,ID_VSPLIT,ID_VPREV,ID_WRAP,ID_ZIN,ID_ZOUT,ID_ZRESET,ID_SBAR,ID_TOP,
 ID_FONT0=200,ID_SIZE0=230,ID_THEME0=250};

typedef struct{const wchar_t*n;COLORREF bg,fg,hd,cd,cb,lk,mu;int dark;}Theme;
static const Theme TH[]={
 {L"Light",RGB(255,255,255),RGB(32,33,36),RGB(0,90,158),RGB(180,50,50),RGB(238,238,238),RGB(0,102,204),RGB(120,120,120),0},
 {L"Sepia",RGB(244,236,216),RGB(67,52,34),RGB(140,70,20),RGB(150,60,30),RGB(230,218,192),RGB(30,90,140),RGB(130,115,95),0},
 {L"Dark",RGB(30,30,30),RGB(212,212,212),RGB(86,156,214),RGB(206,145,120),RGB(50,50,54),RGB(78,201,176),RGB(128,128,128),1},
 {L"Solarized Dark",RGB(0,43,54),RGB(147,161,161),RGB(38,139,210),RGB(203,75,22),RGB(7,54,66),RGB(42,161,152),RGB(101,123,131),1},
 {L"Midnight (OLED black)",RGB(0,0,0),RGB(200,200,200),RGB(120,170,255),RGB(255,160,120),RGB(28,28,28),RGB(100,220,200),RGB(110,110,110),1},
 {L"High Contrast",RGB(0,0,0),RGB(255,255,255),RGB(255,255,0),RGB(0,255,255),RGB(40,40,40),RGB(102,204,255),RGB(200,200,200),1}};
#define NT 6
static const wchar_t*FONTS[]={L"Segoe UI",L"Calibri",L"Cambria",L"Georgia",L"Times New Roman",L"Arial",L"Verdana",L"Consolas",L"Courier New"};
#define NF 9
static const int SIZES[]={9,10,11,12,13,14,16,18,20,24};
#define NS 10

typedef struct{HWND ed;wchar_t fpath[MAX_PATH];int eol,un;}Doc;
#define MAXD 40
static Doc*docs[MAXD];static int nd,cur;
static HINSTANCE hInst;static HWND hWnd,hEd,hPv,hSb,hTab,hFind;
static HMENU mBar,mView,mTheme,mFont,mSize;
static wchar_t ini[MAX_PATH],fontName[LF_FACESIZE]=L"Segoe UI",fbuf[256],rbuf[256],sbt[3][64];
static int mode=1,wrap=1,showSb=1,theme=0,top=0,fontPt=11,pvDirty=1,pvTop=1,dpi=96,splitPct=50,zoomPct=100,drag=0,tabH,gapW,gapX;
static UINT WM_FINDMSG;static FINDREPLACEW fr;
#define path (docs[cur]->fpath)
#define crlf (docs[cur]->eol)
#define KICK SetTimer(hWnd,1,180,0)
#define SM(h,m,w,l) SendMessageW(h,m,(WPARAM)(w),(LPARAM)(l))

/* ---------- settings ---------- */
static void loadini(void){
 GetEnvironmentVariableW(L"APPDATA",ini,MAX_PATH-40);wcscat(ini,L"\\AnnoMD");CreateDirectoryW(ini,0);wcscat(ini,L"\\settings.ini");
 theme=GetPrivateProfileIntW(L"s",L"theme",0,ini);mode=GetPrivateProfileIntW(L"s",L"mode",1,ini);
 wrap=GetPrivateProfileIntW(L"s",L"wrap",1,ini);showSb=GetPrivateProfileIntW(L"s",L"sbar",1,ini);
 top=GetPrivateProfileIntW(L"s",L"top",0,ini);splitPct=GetPrivateProfileIntW(L"s",L"split",50,ini);if(splitPct<15||splitPct>85)splitPct=50;fontPt=GetPrivateProfileIntW(L"s",L"pt",11,ini);
 GetPrivateProfileStringW(L"s",L"font",L"Segoe UI",fontName,LF_FACESIZE,ini);
 if(theme<0||theme>=NT)theme=0;if(mode<0||mode>2)mode=1;if(fontPt<6||fontPt>48)fontPt=11;
}
static void wri(const wchar_t*k,int v){wchar_t s[16];_snwprintf(s,16,L"%d",v);WritePrivateProfileStringW(L"s",k,s,ini);}
static void saveini(void){wri(L"theme",theme);wri(L"mode",mode);wri(L"wrap",wrap);wri(L"sbar",showSb);wri(L"top",top);wri(L"split",splitPct);wri(L"pt",fontPt);
 WritePrivateProfileStringW(L"s",L"font",fontName,ini);}

/* ---------- text helpers ---------- */
static wchar_t*get_text(int*len,int usecrlf){
 GETTEXTLENGTHEX gl={(usecrlf?GTL_USECRLF:0)|GTL_NUMCHARS,1200};
 int n=(int)SM(hEd,EM_GETTEXTLENGTHEX,&gl,0);wchar_t*b=malloc((n+1)*sizeof(wchar_t));
 GETTEXTEX g={(n+1)*sizeof(wchar_t),usecrlf?GT_USECRLF:GT_DEFAULT,1200,0,0};
 SM(hEd,EM_GETTEXTEX,&g,b);b[n]=0;*len=n;return b;}
static void set_text(const wchar_t*w){SETTEXTEX st={ST_DEFAULT,1200};SM(hEd,EM_SETTEXTEX,&st,w);}

/* ---------- Markdown -> RTF (preview) ---------- */
typedef struct{char*p;size_t n,c;}Buf;
static void bp(Buf*b,const char*s,size_t l){if(b->n+l+1>b->c){b->c=(b->n+l+1)*2+256;b->p=realloc(b->p,b->c);}memcpy(b->p+b->n,s,l);b->n+=l;b->p[b->n]=0;}
static void bs(Buf*b,const char*s){bp(b,s,strlen(s));}
static void bf(Buf*b,const char*f,...){char t[640];va_list a;va_start(a,f);vsnprintf(t,sizeof t,f,a);va_end(a);bs(b,t);}
static void bw(Buf*b,wchar_t c){
 if(c=='\\'||c=='{'||c=='}'){char t[3]={'\\',(char)c,0};bs(b,t);}
 else if(c=='\t')bs(b,"\\tab ");
 else if(c>=32&&c<127){char t[2]={(char)c,0};bs(b,t);}
 else if(c>=127)bf(b,"\\u%d?",(short)c);}

static void inl(Buf*b,const wchar_t*s,int n){
 int B=0,I=0,S=0,i,j,k;bs(b,"{");
 for(i=0;i<n;i++){wchar_t c=s[i];
  if(c=='\\'&&i+1<n&&iswpunct(s[i+1])){bw(b,s[++i]);continue;}
  if(c=='`'){for(j=i+1;j<n&&s[j]!='`';j++);
   if(j<n){bs(b,"{\\f1\\cf3\\highlight5 \\u160?");for(k=i+1;k<j;k++)bw(b,s[k]);bs(b,"\\u160?}");i=j;continue;}}
  if(c=='['||(c=='!'&&i+1<n&&s[i+1]=='[')){
   int im=c=='!',a=i+im+1,e=a;while(e<n&&s[e]!=']')e++;
   if(e+1<n&&s[e+1]=='('){int u=e+2;while(u<n&&s[u]!=')')u++;
    if(u<n){bs(b,im?"{\\i\\cf6 [image: ":"{\\ul\\cf4 ");for(k=a;k<e;k++)bw(b,s[k]);bs(b,im?"]}":"}");i=u;continue;}}}
  if(c=='~'&&i+1<n&&s[i+1]=='~'){S=!S;bs(b,S?"\\strike ":"\\strike0 ");i++;continue;}
  if(c=='*'||c=='_'){
   k=1;while(i+k<n&&s[i+k]==c)k++;
   wchar_t pv=i?s[i-1]:' ',nx=i+k<n?s[i+k]:' ';
   int co=!iswspace(nx)&&(c=='*'||!iswalnum(pv)),cc=!iswspace(pv)&&(c=='*'||!iswalnum(nx));
   if(k<=3){int on=k==1?I:k==2?B:(B&&I);
    if(on?cc:co){int nb=k>=2,ni=k!=2;
     if(on){if(nb)B=0;if(ni)I=0;}else{if(nb)B=1;if(ni)I=1;}
     if(nb)bs(b,B?"\\b ":"\\b0 ");if(ni)bs(b,I?"\\i ":"\\i0 ");
     i+=k-1;continue;}}
   for(j=0;j<k;j++)bw(b,c);i+=k-1;continue;}
  bw(b,c);}
 bs(b,"}");}

#define RGBC(x) GetRValue(x),GetGValue(x),GetBValue(x)
static void cardend(Buf*b,int cl,int bfs){bf(b,"%s\\fs8 \\cell\\row\\pard\\sa0\\f0\\fs10\\par\\pard\\f0\\fs%d\\cf1 ",cl?"\\par ":"",bfs);}
static char*md2rtf(const wchar_t*t,int len,size_t*outn,int cw){
 const Theme*T=&TH[theme];Buf b={0};int pa=0,code=0,cl=0,i=0,k,cfs=fontPt*9/5,bfs=fontPt*2;
 bs(&b,"{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0\\fnil\\fcharset1 ");
 for(const wchar_t*q=fontName;*q;q++)bw(&b,*q);
 bs(&b,";}{\\f1\\fmodern\\fcharset1 Consolas;}}");
 bf(&b,"{\\colortbl;\\red%d\\green%d\\blue%d;\\red%d\\green%d\\blue%d;\\red%d\\green%d\\blue%d;\\red%d\\green%d\\blue%d;\\red%d\\green%d\\blue%d;\\red%d\\green%d\\blue%d;}\\viewkind4\\uc1\\f0\\fs%d\\cf1 ",
  RGBC(T->fg),RGBC(T->hd),RGBC(T->cd),RGBC(T->lk),RGBC(T->cb),RGBC(T->mu),fontPt*2);
#define CL if(pa){bs(&b,"\\par ");pa=0;}
 while(i<len){
  int j=i;while(j<len&&t[j]!='\r'&&t[j]!='\n')j++;
  const wchar_t*ln=t+i;int n=j-i;
  i=j;if(i<len&&t[i]=='\r')i++;if(i<len&&t[i]=='\n')i++;
  int ind=0;while(ind<n&&ln[ind]==' ')ind++;
  const wchar_t*p=ln+ind;int m=n-ind;
  if(m>=3&&(!wcsncmp(p,L"```",3)||!wcsncmp(p,L"~~~",3))){CL
   if(code){cardend(&b,cl,bfs);code=0;}
   else{int q=3;while(q<m&&p[q]==' ')q++;int e=q;while(e<m&&p[e]!=' '&&e-q<24)e++;
    bf(&b,"\\trowd\\trgaph120\\trleft60\\clbrdrt\\brdrs\\brdrw20\\brdrcf6\\clbrdrl\\brdrs\\brdrw20\\brdrcf6\\clbrdrb\\brdrs\\brdrw20\\brdrcf6\\clbrdrr\\brdrs\\brdrw20\\brdrcf6\\clcbpat5\\cellx%d\\pard\\intbl\\sa0\\sb0\\f1\\fs8 \\par\\fs%d\\cf1 ",cw-200,cfs);
    cl=0;if(e>q){bs(&b,"{\\f0\\b\\cf6\\fs16 ");for(k=q;k<e;k++)bw(&b,p[k]);bs(&b,"}");cl=1;}
    code=1;}
   continue;}
  if(code){if(cl++)bs(&b,"\\par ");for(k=0;k<n;k++)bw(&b,ln[k]);continue;}
  if(!m){CL continue;}
  {int h=0;while(h<m&&p[h]=='#')h++;
   if(h>=1&&h<=6&&(h==m||p[h]==' ')){CL
    static const int pc[]={0,190,160,135,120,110,100};int q=h;while(q<m&&p[q]==' ')q++;
    bf(&b,"\\pard\\sb220\\sa100\\keepn{\\b\\cf2\\fs%d ",fontPt*2*pc[h]/100);inl(&b,p+q,m-q);bs(&b,"}\\par ");continue;}}
  if(m>=3&&(p[0]=='-'||p[0]=='*'||p[0]=='_')){int ok=1,cnt=0;
   for(k=0;k<m;k++){if(p[k]==p[0])cnt++;else if(p[k]!=' ')ok=0;}
   if(ok&&cnt>=3){CL bs(&b,"\\pard\\sa120{\\cf6\\fs18 ");for(k=0;k<60;k++)bs(&b,"\\u9472?");bs(&b,"}\\par ");continue;}}
  if(p[0]=='>'){CL int q=1;while(q<m&&p[q]==' ')q++;
   bs(&b,"\\pard\\li360\\fi-240\\sa40{\\cf4\\u9474?}\\tab {\\i\\cf6 ");inl(&b,p+q,m-q);bs(&b,"}\\par ");continue;}
  {int g=0,s0=2;
   if(m>=2&&(p[0]=='-'||p[0]=='*'||p[0]=='+')&&p[1]==' '){
    g=ind>=2?9702:8226;
    if(m>=6&&p[2]=='['&&(p[3]==' '||p[3]=='x'||p[3]=='X')&&p[4]==']'&&p[5]==' '){g=p[3]==' '?9744:9745;s0=6;}
    CL bf(&b,"\\pard\\li%d\\fi-300\\sa40{\\cf2 \\u%d?}\\tab ",500+ind*120,g);inl(&b,p+s0,m-s0);bs(&b,"\\par ");continue;}
   int d=0;while(d<m&&iswdigit(p[d]))d++;
   if(d&&d<9&&d+1<m&&(p[d]=='.'||p[d]==')')&&p[d+1]==' '){CL
    bf(&b,"\\pard\\li%d\\fi-360\\sa40{\\b\\cf2 ",520+ind*120);for(k=0;k<=d;k++)bw(&b,p[k]);bs(&b,"}\\tab ");inl(&b,p+d+2,m-d-2);bs(&b,"\\par ");continue;}}
  if(p[0]=='|'){CL int sep=1;for(k=0;k<m;k++)if(!wcschr(L"|-: ",p[k]))sep=0;if(sep)continue;
   bf(&b,"\\pard\\li120\\sa0{\\f1\\fs%d ",fontPt*9/5);for(k=0;k<m;k++)bw(&b,p[k]);bs(&b,"}\\par ");continue;}
  if(pa)bs(&b," ");else{bs(&b,"\\pard\\sa140\\sl290\\slmult1 ");pa=1;}
  inl(&b,p,m);}
 CL if(code)cardend(&b,cl,bfs);bs(&b,"}");*outn=b.n;return b.p;}

typedef struct{const char*s;LONG n,pos;}RS;
static DWORD CALLBACK rdcb(DWORD_PTR c,LPBYTE buf,LONG cb,LONG*got){RS*r=(RS*)c;LONG k=r->n-r->pos;if(k>cb)k=cb;memcpy(buf,r->s+r->pos,k);r->pos+=k;*got=k;return 0;}
static void refresh_preview(void){
 pvDirty=0;if(mode==0||!nd)return;
 int n;wchar_t*t=get_text(&n,0);size_t rn;char*r;
 RECT cr;GetClientRect(hPv,&cr);int cw=(cr.right-40*dpi/96)*1440/dpi;if(cw<2400)cw=2400;
 if(n>1500000){const wchar_t*msg=L"Preview is disabled above 1.5 MB to keep memory low.";r=md2rtf(msg,(int)wcslen(msg),&rn,cw);}
 else r=md2rtf(t,n,&rn,cw);
 POINT pt={0,0};if(!pvTop)SM(hPv,EM_GETSCROLLPOS,0,&pt);pvTop=0;SM(hPv,WM_SETREDRAW,0,0);
 RS rs={r,(LONG)rn,0};EDITSTREAM es={(DWORD_PTR)&rs,0,rdcb};
 SM(hPv,EM_STREAMIN,SF_RTF,&es);SM(hPv,EM_SETSCROLLPOS,0,&pt);SM(hPv,WM_SETREDRAW,1,0);InvalidateRect(hPv,0,1);
 free(r);free(t);}

/* ---------- ui state / documents / tabs ---------- */
static int ask_save(void);static int load(const wchar_t*f);static int save(int as);static void cmd(int id);
static void sync(void){
 CheckMenuRadioItem(mView,ID_VEDIT,ID_VPREV,ID_VEDIT+mode,MF_BYCOMMAND);
 CheckMenuRadioItem(mTheme,ID_THEME0,ID_THEME0+NT-1,ID_THEME0+theme,MF_BYCOMMAND);
 for(int i=0;i<NF;i++)CheckMenuItem(mFont,ID_FONT0+i,MF_BYCOMMAND|(!wcscmp(fontName,FONTS[i])?MF_CHECKED:MF_UNCHECKED));
 for(int i=0;i<NS;i++)CheckMenuItem(mSize,ID_SIZE0+i,MF_BYCOMMAND|(fontPt==SIZES[i]?MF_CHECKED:MF_UNCHECKED));
 CheckMenuItem(mView,ID_WRAP,MF_BYCOMMAND|(wrap?MF_CHECKED:MF_UNCHECKED));
 CheckMenuItem(mView,ID_SBAR,MF_BYCOMMAND|(showSb?MF_CHECKED:MF_UNCHECKED));
 CheckMenuItem(mView,ID_TOP,MF_BYCOMMAND|(top?MF_CHECKED:MF_UNCHECKED));}
static void layout(void){
 if(!hSb||!hTab)return;RECT r;GetClientRect(hWnd,&r);int sbh=0,w=r.right;tabH=30*dpi/96;gapW=6*dpi/96;
 if(showSb){SM(hSb,WM_SIZE,0,0);RECT s;GetWindowRect(hSb,&s);sbh=s.bottom-s.top;ShowWindow(hSb,SW_SHOW);
  int p[3]={w/3,w*2/3,-1};SM(hSb,SB_SETPARTS,3,p);}else ShowWindow(hSb,SW_HIDE);
 int h=r.bottom-sbh-tabH;if(h<0)h=0;
 int ew=mode==0?w:mode==1?(w-gapW)*splitPct/100:0,px=mode==0?w:mode==1?ew+gapW:0,pw=mode==0?0:w-px;
 gapX=ew;MoveWindow(hTab,0,0,w,tabH,1);
 if(hEd){MoveWindow(hEd,0,tabH,ew,h,1);ShowWindow(hEd,ew?SW_SHOW:SW_HIDE);}
 MoveWindow(hPv,px,tabH,pw,h,1);ShowWindow(hPv,pw?SW_SHOW:SW_HIDE);
 if(mode){pvDirty=1;KICK;}}
static void style_ed(HWND e){
 const Theme*T=&TH[theme];CHARFORMAT2W cf;memset(&cf,0,sizeof cf);cf.cbSize=sizeof cf;
 cf.dwMask=CFM_COLOR|CFM_FACE|CFM_SIZE|CFM_BOLD|CFM_ITALIC;cf.yHeight=fontPt*20;cf.crTextColor=T->fg;cf.bCharSet=DEFAULT_CHARSET;
 wcscpy(cf.szFaceName,fontName);SM(e,EM_SETBKGNDCOLOR,0,T->bg);SM(e,EM_SETCHARFORMAT,SCF_ALL,&cf);}
static void style(void){
 const Theme*T=&TH[theme];for(int i=0;i<nd;i++)style_ed(docs[i]->ed);
 SM(hPv,EM_SETBKGNDCOLOR,0,T->bg);BOOL d=T->dark;DwmSetWindowAttribute(hWnd,20,&d,sizeof d);
 SM(hSb,SB_SETBKCOLOR,0,T->bg);InvalidateRect(hWnd,0,1);InvalidateRect(hTab,0,1);sync();refresh_preview();}
static int moded(int i){return (int)SM(docs[i]->ed,EM_GETMODIFY,0,0);}
static void docname(int i,wchar_t*o){Doc*d=docs[i];
 if(d->fpath[0]){const wchar_t*s=wcsrchr(d->fpath,'\\');wcscpy(o,s?s+1:d->fpath);}else _snwprintf(o,32,L"Untitled%d",d->un);}
static void title(void){
 if(!nd)return;wchar_t n[MAX_PATH],t[MAX_PATH+64];docname(cur,n);
 t[0]=0;if(moded(cur))wcscat(t,L"*");wcscat(t,n);wcscat(t,L" - " APP);SetWindowTextW(hWnd,t);InvalidateRect(hTab,0,0);}
static void sbset(int i,const wchar_t*s){wcsncpy(sbt[i],s,63);SM(hSb,SB_SETTEXTW,i|SBT_OWNERDRAW,sbt[i]);}
static void status(void){
 if(!nd)return;
 int n,ln=1,col=1,w=0,inw=0;wchar_t*t=get_text(&n,0);CHARRANGE c;SM(hEd,EM_EXGETSEL,0,&c);wchar_t s[64];
 for(int i=0;i<n;i++){if(i<c.cpMin){if(t[i]=='\r'){ln++;col=1;}else col++;}
  if(iswspace(t[i]))inw=0;else if(!inw){inw=1;w++;}}
 free(t);
 _snwprintf(s,64,L"Ln %d, Col %d",ln,col);sbset(0,s);
 _snwprintf(s,64,L"%d words, %d chars",w,n);sbset(1,s);
 sbset(2,crlf?L"UTF-8, CRLF":L"UTF-8, LF");}
static void zoom(int d){
 DWORD n=0,dn=0;SM(hEd,EM_GETZOOM,&n,&dn);int z=(n&&dn)?(int)(n*100/dn):100;
 z=d?z+d:100;if(z<20)z=20;if(z>500)z=500;zoomPct=z;
 for(int i=0;i<nd;i++)SM(docs[i]->ed,EM_SETZOOM,z,100);SM(hPv,EM_SETZOOM,z,100);}

/* right-click / Menu-key context menu for the editor and the preview */
#define G(b) ((b)?0:MF_GRAYED)
static void ctxmenu(HWND h,int x,int y){
 int pv=(h==hPv);CHARRANGE c;SM(h,EM_EXGETSEL,0,&c);int sel=c.cpMax>c.cpMin;HMENU m=CreatePopupMenu();
 if(!pv){AppendMenuW(m,MF_STRING|G(SM(h,EM_CANUNDO,0,0)),ID_UNDO,L"&Undo\tCtrl+Z");
  AppendMenuW(m,MF_STRING|G(SM(h,EM_CANREDO,0,0)),ID_REDO,L"&Redo\tCtrl+Y");AppendMenuW(m,MF_SEPARATOR,0,0);
  AppendMenuW(m,MF_STRING|G(sel),ID_CUT,L"Cu&t\tCtrl+X");}
 AppendMenuW(m,MF_STRING|G(sel),ID_COPY,L"&Copy\tCtrl+C");
 if(!pv){AppendMenuW(m,MF_STRING|G(SM(h,EM_CANPASTE,0,0)),ID_PASTE,L"&Paste\tCtrl+V");
  AppendMenuW(m,MF_STRING|G(sel),ID_DELETE,L"&Delete\tDel");}
 AppendMenuW(m,MF_SEPARATOR,0,0);AppendMenuW(m,MF_STRING,ID_SELALL,L"Select &All\tCtrl+A");
 if(!pv){AppendMenuW(m,MF_SEPARATOR,0,0);AppendMenuW(m,MF_STRING,ID_BOLD,L"&Bold\tCtrl+B");
  AppendMenuW(m,MF_STRING,ID_ITALIC,L"&Italic\tCtrl+I");AppendMenuW(m,MF_STRING,ID_CODE,L"Inline C&ode");
  AppendMenuW(m,MF_STRING,ID_LINK,L"&Link\tCtrl+K");}
 SetFocus(h);int id=TrackPopupMenu(m,TPM_RETURNCMD|TPM_RIGHTBUTTON,x,y,0,hWnd,0);DestroyMenu(m);if(id)cmd(id);}
static LRESULT CALLBACK EP(HWND h,UINT m,WPARAM w,LPARAM l,UINT_PTR id,DWORD_PTR ref){
 if(m==WM_RBUTTONUP){POINTL pt={(short)LOWORD(l),(short)HIWORD(l)};
  if(h==hEd){CHARRANGE c;SM(h,EM_EXGETSEL,0,&c);LONG ix=(LONG)SM(h,EM_CHARFROMPOS,0,&pt);   /* click outside selection -> move caret */
   if(ix<c.cpMin||ix>c.cpMax){CHARRANGE z={ix,ix};SM(h,EM_EXSETSEL,0,&z);}}
  POINT sp={pt.x,pt.y};ClientToScreen(h,&sp);ctxmenu(h,sp.x,sp.y);return 0;}
 if(m==WM_CONTEXTMENU){int x=(short)LOWORD(l),y=(short)HIWORD(l);
  if(l==-1){POINT p={20,20};if(h==hEd){CHARRANGE c;SM(h,EM_EXGETSEL,0,&c);SM(h,EM_POSFROMCHAR,&p,c.cpMin);}ClientToScreen(h,&p);x=p.x;y=p.y;}
  ctxmenu(h,x,y);return 0;}
 return DefSubclassProc(h,m,w,l);}

/* one RichEdit per document: each tab keeps its own text, undo history, caret and scroll */
static HWND mked(void){
 HWND e=CreateWindowExW(0,L"RICHEDIT50W",L"",WS_CHILD|WS_VSCROLL|WS_HSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|ES_AUTOHSCROLL|ES_NOHIDESEL|ES_WANTRETURN,
  0,0,0,0,hWnd,(HMENU)1,hInst,0);int mg=12*dpi/96;
 SM(e,EM_SETTEXTMODE,TM_PLAINTEXT|TM_MULTILEVELUNDO,0);SM(e,EM_EXLIMITTEXT,0,0x7FFFFFF0);SM(e,EM_SETUNDOLIMIT,200,0);
 SM(e,EM_SETEVENTMASK,0,ENM_CHANGE|ENM_SELCHANGE|ENM_DROPFILES);SM(e,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,MAKELONG(mg,mg));
 DragAcceptFiles(e,1);SM(e,EM_SETTARGETDEVICE,0,wrap?0:1);if(zoomPct!=100)SM(e,EM_SETZOOM,zoomPct,100);
 SetWindowSubclass(e,EP,1,0);style_ed(e);return e;}
static void switchdoc(int i){
 if(i<0||i>=nd)return;cur=i;hEd=docs[i]->ed;
 for(int j=0;j<nd;j++)if(j!=i)ShowWindow(docs[j]->ed,SW_HIDE);
 layout();pvDirty=1;pvTop=1;title();status();refresh_preview();SetFocus(mode==2?hPv:hEd);}
static int newdoc(void){
 if(nd>=MAXD){MessageBoxW(hWnd,L"Too many tabs open.",APP,MB_ICONINFORMATION);return 0;}
 Doc*d=calloc(1,sizeof(Doc));d->ed=mked();d->eol=1;int un=1;
 for(;;un++){int used=0;for(int i=0;i<nd;i++)if(!docs[i]->fpath[0]&&docs[i]->un==un)used=1;if(!used)break;}
 d->un=un;docs[nd++]=d;switchdoc(nd-1);return 1;}
static void dropdoc(int i){
 DestroyWindow(docs[i]->ed);free(docs[i]);memmove(docs+i,docs+i+1,(nd-i-1)*sizeof(Doc*));nd--;
 if(!nd){hEd=0;newdoc();}else switchdoc(i<nd?i:nd-1);}
static void closedoc(int i){if(i<0||i>=nd)return;if(i!=cur)switchdoc(i);if(ask_save())dropdoc(i);}
static int ask_all(void){for(int i=0;i<nd;i++)if(moded(i)){switchdoc(i);if(!ask_save())return 0;}return 1;}
static void openpath(const wchar_t*f0){
 wchar_t f[MAX_PATH];if(!GetFullPathNameW(f0,MAX_PATH,f,0))wcsncpy(f,f0,MAX_PATH-1);
 for(int j=0;j<nd;j++)if(docs[j]->fpath[0]&&!_wcsicmp(docs[j]->fpath,f)){switchdoc(j);return;}
 int reuse=nd&&!path[0]&&!SM(hEd,EM_GETMODIFY,0,0)&&GetWindowTextLengthW(hEd)==0;
 if(!reuse&&!newdoc())return;
 if(!load(f)&&!reuse)dropdoc(cur);}
static void dropfiles(HDROP d){
 UINT n=DragQueryFileW(d,0xFFFFFFFF,0,0);
 for(UINT i=0;i<n;i++){wchar_t f[MAX_PATH];DragQueryFileW(d,i,f,MAX_PATH);openpath(f);}}

/* custom-drawn, theme-aware tab strip */
static HFONT tabFont;
static int tabw(int cw){int w=(cw-32*dpi/96)/(nd?nd:1),mx=190*dpi/96,mn=70*dpi/96;return w>mx?mx:w<mn?mn:w;}
static LRESULT CALLBACK TP(HWND h,UINT m,WPARAM w,LPARAM l){
 RECT r;GetClientRect(h,&r);int tw=tabw(r.right),xw=24*dpi/96,pw=30*dpi/96;
 switch(m){
 case WM_ERASEBKGND:return 1;
 case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);const Theme*T=&TH[theme];
  HDC mem=CreateCompatibleDC(dc);HBITMAP bm=CreateCompatibleBitmap(dc,r.right,r.bottom);HGDIOBJ ob=SelectObject(mem,bm);
  HBRUSH b=CreateSolidBrush(T->cb);FillRect(mem,&r,b);DeleteObject(b);SelectObject(mem,tabFont);SetBkMode(mem,TRANSPARENT);
  for(int i=0;i<nd;i++){RECT t={i*tw,0,(i+1)*tw,r.bottom};
   if(i==cur){b=CreateSolidBrush(T->bg);FillRect(mem,&t,b);DeleteObject(b);RECT a=t;a.bottom=3*dpi/96;b=CreateSolidBrush(T->hd);FillRect(mem,&a,b);DeleteObject(b);}
   wchar_t s[MAX_PATH+8];docname(i,s);if(moded(i))wcscat(s,L" \x25CF");
   RECT x=t;x.left+=10*dpi/96;x.right-=xw;SetTextColor(mem,i==cur?T->fg:T->mu);
   DrawTextW(mem,s,-1,&x,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);
   RECT c=t;c.left=t.right-xw;DrawTextW(mem,L"\x00D7",-1,&c,DT_SINGLELINE|DT_VCENTER|DT_CENTER);}
  RECT pl={nd*tw,0,nd*tw+pw,r.bottom};SetTextColor(mem,T->fg);DrawTextW(mem,L"+",-1,&pl,DT_SINGLELINE|DT_VCENTER|DT_CENTER);
  BitBlt(dc,0,0,r.right,r.bottom,mem,0,0,SRCCOPY);SelectObject(mem,ob);DeleteObject(bm);DeleteDC(mem);EndPaint(h,&ps);return 0;}
 case WM_RBUTTONUP:{int x=(short)LOWORD(l),i=x/tw;if(i>=nd)return 0;
  HMENU pm=CreatePopupMenu();AppendMenuW(pm,MF_STRING,1,L"&Close");AppendMenuW(pm,MF_STRING|G(nd>1),2,L"Close &Others");
  AppendMenuW(pm,MF_STRING,3,L"&Save");AppendMenuW(pm,MF_STRING|G(docs[i]->fpath[0]),4,L"Copy Full &Path");
  POINT pt={x,(short)HIWORD(l)};ClientToScreen(h,&pt);int cid=TrackPopupMenu(pm,TPM_RETURNCMD|TPM_RIGHTBUTTON,pt.x,pt.y,0,h,0);DestroyMenu(pm);
  if(cid==1)closedoc(i);
  else if(cid==2){Doc*keep=docs[i];for(int j=0;j<nd;){if(docs[j]==keep){j++;continue;}int b4=nd;closedoc(j);if(nd==b4)break;}
   for(int j=0;j<nd;j++)if(docs[j]==keep){switchdoc(j);break;}}
  else if(cid==3){switchdoc(i);save(0);}
  else if(cid==4){const wchar_t*s=docs[i]->fpath;size_t n=(wcslen(s)+1)*sizeof(wchar_t);HGLOBAL g=GlobalAlloc(GMEM_MOVEABLE,n);
   memcpy(GlobalLock(g),s,n);GlobalUnlock(g);if(OpenClipboard(h)){EmptyClipboard();SetClipboardData(CF_UNICODETEXT,g);CloseClipboard();}else GlobalFree(g);}
  return 0;}
 case WM_LBUTTONDOWN:case WM_MBUTTONDOWN:{int x=(short)LOWORD(l),i=x/tw;
  if(i<nd){if(m==WM_MBUTTONDOWN||x>=(i+1)*tw-xw)closedoc(i);else switchdoc(i);}
  else if(x<nd*tw+pw)newdoc();
  return 0;}}
 return DefWindowProcW(h,m,w,l);}

/* ---------- files ---------- */
static const wchar_t FILT[]=L"Markdown (*.md;*.markdown;*.txt)\0*.md;*.markdown;*.txt\0All files\0*.*\0";
static int load(const wchar_t*f){
 HANDLE h=CreateFileW(f,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,0,0);
 if(h==INVALID_HANDLE_VALUE){MessageBoxW(hWnd,L"Cannot open file.",APP,MB_ICONERROR);return 0;}
 DWORD sz=GetFileSize(h,0),rd=0;char*b=malloc(sz+4);ReadFile(h,b,sz,&rd,0);CloseHandle(h);memset(b+rd,0,4);
 wchar_t*w,*own=0;int n;
 if(rd>=2&&(BYTE)b[0]==0xFF&&(BYTE)b[1]==0xFE){w=(wchar_t*)(b+2);n=(rd-2)/2;w[n]=0;}
 else{int sk=(rd>=3&&(BYTE)b[0]==0xEF&&(BYTE)b[1]==0xBB&&(BYTE)b[2]==0xBF)?3:0;UINT cp=CP_UTF8;
  n=MultiByteToWideChar(cp,MB_ERR_INVALID_CHARS,b+sk,rd-sk,0,0);
  if(!n&&rd>(DWORD)sk){cp=CP_ACP;n=MultiByteToWideChar(cp,0,b+sk,rd-sk,0,0);}
  own=w=malloc((n+1)*sizeof(wchar_t));MultiByteToWideChar(cp,0,b+sk,rd-sk,w,n);w[n]=0;}
 crlf=wcsstr(w,L"\r\n")!=0||!wcschr(w,'\n');
 set_text(w);free(own);free(b);
 SM(hEd,EM_SETMODIFY,0,0);SM(hEd,EM_EMPTYUNDOBUFFER,0,0);SM(hEd,EM_SETSEL,0,0);
 wcsncpy(path,f,MAX_PATH-1);pvDirty=1;pvTop=1;title();status();refresh_preview();return 1;}
static int save(int as){
 if(as||!path[0]){wchar_t f[MAX_PATH];wcscpy(f,path);OPENFILENAMEW o;memset(&o,0,sizeof o);o.lStructSize=sizeof o;o.hwndOwner=hWnd;
  o.lpstrFilter=FILT;o.lpstrFile=f;o.nMaxFile=MAX_PATH;o.lpstrDefExt=L"md";o.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST;
  if(!GetSaveFileNameW(&o))return 0;wcscpy(path,f);}
 int n;wchar_t*t=get_text(&n,crlf);if(!crlf)for(int i=0;i<n;i++)if(t[i]=='\r')t[i]='\n';
 int m=WideCharToMultiByte(CP_UTF8,0,t,n,0,0,0,0);char*u=malloc(m+1);WideCharToMultiByte(CP_UTF8,0,t,n,u,m,0,0);
 HANDLE h=CreateFileW(path,GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);DWORD wr=0;int ok=h!=INVALID_HANDLE_VALUE&&WriteFile(h,u,m,&wr,0);
 if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);free(u);free(t);
 if(!ok){MessageBoxW(hWnd,L"Could not save the file.",APP,MB_ICONERROR);return 0;}
 SM(hEd,EM_SETMODIFY,0,0);title();return 1;}
static int ask_save(void){
 if(!SM(hEd,EM_GETMODIFY,0,0))return 1;
 int r=MessageBoxW(hWnd,L"Save changes to this document?",APP,MB_YESNOCANCEL|MB_ICONQUESTION);
 return r==IDCANCEL?0:r==IDYES?save(0):1;}
static void openfile(void){
 wchar_t*f=calloc(32768,sizeof(wchar_t));OPENFILENAMEW o;memset(&o,0,sizeof o);o.lStructSize=sizeof o;o.hwndOwner=hWnd;
 o.lpstrFilter=FILT;o.lpstrFile=f;o.nMaxFile=32768;o.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_ALLOWMULTISELECT|OFN_EXPLORER;
 if(GetOpenFileNameW(&o)){size_t dl=wcslen(f);wchar_t*q=f+dl+1;
  if(!*q)openpath(f);
  else while(*q){wchar_t full[MAX_PATH];size_t ql=wcslen(q);
   if(dl+ql+2<MAX_PATH){wcscpy(full,f);wcscat(full,L"\\");wcscat(full,q);openpath(full);}q+=ql+1;}}
 free(f);}

/* ---------- Format / Paragraph ---------- */
static void wrapsel(const wchar_t*lf,const wchar_t*rt){
 CHARRANGE c;SM(hEd,EM_EXGETSEL,0,&c);int ll=(int)wcslen(lf),rl=(int)wcslen(rt),m=c.cpMax-c.cpMin,k;
 wchar_t*o=malloc((m+ll+rl+2)*sizeof(wchar_t));
 if(!m){wcscpy(o,lf);wcscat(o,rt);SM(hEd,EM_REPLACESEL,TRUE,o);
  CHARRANGE z={c.cpMin+ll,c.cpMin+ll};SM(hEd,EM_EXSETSEL,0,&z);free(o);return;}
 int n;wchar_t*t=get_text(&n,0);const wchar_t*s=t+c.cpMin;
 int un=m>=ll+rl&&!wcsncmp(s,lf,ll)&&!wcsncmp(s+m-rl,rt,rl)&&!(ll==1&&m>1&&s[1]==lf[0]);
 if(un){wcsncpy(o,s+ll,m-ll-rl);k=m-ll-rl;}else{wcscpy(o,lf);wcsncpy(o+ll,s,m);wcscpy(o+ll+m,rt);k=ll+m+rl;}
 o[k]=0;SM(hEd,EM_REPLACESEL,TRUE,o);CHARRANGE z={c.cpMin,c.cpMin+k};SM(hEd,EM_EXSETSEL,0,&z);free(o);free(t);}

static void blocks(int op){   /* 1-6 heading, 0 normal, 10 quote, 11 bullet, 12 number, 13 task, 20 indent, 21 outdent */
 CHARRANGE c;SM(hEd,EM_EXGETSEL,0,&c);int n;wchar_t*t=get_text(&n,0);
 int s=c.cpMin,e=c.cpMax,lines=1;while(s>0&&t[s-1]!='\r')s--;
 if(e>c.cpMin&&t[e-1]=='\r')e--;while(e<n&&t[e]!='\r')e++;
 for(int i=s;i<e;i++)if(t[i]=='\r')lines++;
 wchar_t*o=malloc(((e-s)+lines*16+16)*sizeof(wchar_t));int k=0,num=1;const wchar_t*p=t+s,*end=t+e;
 for(;;){
  const wchar_t*q=p;while(q<end&&*q!='\r')q++;
  int ln=(int)(q-p),ind=0;while(ind<ln&&p[ind]==' ')ind++;
  const wchar_t*b=p+ind;int bl=ln-ind,kind=0,pl=0,h=0;
  while(h<bl&&b[h]=='#')h++;
  if(h>=1&&h<=6&&h<bl&&b[h]==' '){kind=h;pl=h+1;}
  else if(bl>=1&&b[0]=='>'){kind=10;pl=(bl>1&&b[1]==' ')?2:1;}
  else if(bl>=2&&(b[0]=='-'||b[0]=='*'||b[0]=='+')&&b[1]==' '){
   if(bl>=6&&b[2]=='['&&(b[3]==' '||b[3]=='x'||b[3]=='X')&&b[4]==']'&&b[5]==' '){kind=13;pl=6;}else{kind=11;pl=2;}}
  else{int d=0;while(d<bl&&iswdigit(b[d]))d++;if(d&&d<9&&d+1<bl&&b[d]=='.'&&b[d+1]==' '){kind=12;pl=d+2;}}
  if(op==20){o[k++]=' ';o[k++]=' ';memcpy(o+k,p,ln*sizeof(wchar_t));k+=ln;}
  else if(op==21){int r=0;if(ln&&p[0]=='\t')r=1;else while(r<2&&r<ln&&p[r]==' ')r++;memcpy(o+k,p+r,(ln-r)*sizeof(wchar_t));k+=ln-r;}
  else{for(int i=0;i<ind;i++)o[k++]=' ';
   if(op&&kind!=op){
    if(op<=6){for(int i=0;i<op;i++)o[k++]='#';o[k++]=' ';}
    else if(op==10){o[k++]='>';o[k++]=' ';}
    else if(op==11){o[k++]='-';o[k++]=' ';}
    else if(op==12)k+=_snwprintf(o+k,16,L"%d. ",num);
    else if(op==13){wcscpy(o+k,L"- [ ] ");k+=6;}}
   memcpy(o+k,b+pl,(bl-pl)*sizeof(wchar_t));k+=bl-pl;num++;}
  if(q>=end)break;o[k++]='\r';p=q+1;}
 o[k]=0;CHARRANGE r={s,e};SM(hEd,EM_EXSETSEL,0,&r);SM(hEd,EM_REPLACESEL,TRUE,o);
 CHARRANGE r2={s,s+k};SM(hEd,EM_EXSETSEL,0,&r2);free(o);free(t);}

/* ---------- Find / Replace ---------- */
static int dofind(DWORD fl,int msg){
 if(!fbuf[0])return 0;CHARRANGE c;SM(hEd,EM_EXGETSEL,0,&c);FINDTEXTEXW ft;memset(&ft,0,sizeof ft);ft.lpstrText=fbuf;
 int down=(fl&FR_DOWN)!=0;DWORD ff=(fl&(FR_MATCHCASE|FR_WHOLEWORD))|(down?FR_DOWN:0);
 ft.chrg.cpMin=down?c.cpMax:c.cpMin;ft.chrg.cpMax=down?-1:0;
 LRESULT r=SM(hEd,EM_FINDTEXTEXW,ff,&ft);
 if(r<0&&msg){ft.chrg.cpMin=down?0:GetWindowTextLengthW(hEd);ft.chrg.cpMax=down?-1:0;r=SM(hEd,EM_FINDTEXTEXW,ff,&ft);}
 if(r<0){if(msg)MessageBoxW(hFind?hFind:hWnd,L"Not found.",APP,MB_ICONINFORMATION);return 0;}
 SM(hEd,EM_EXSETSEL,0,&ft.chrgText);SM(hEd,EM_SCROLLCARET,0,0);return 1;}
static void doreplace(DWORD fl,int all){
 if(!fbuf[0])return;
 if(all){int cnt=0;CHARRANGE z={0,0};SM(hEd,EM_EXSETSEL,0,&z);SM(hEd,WM_SETREDRAW,0,0);
  while(dofind(FR_DOWN|(fl&(FR_MATCHCASE|FR_WHOLEWORD)),0)){SM(hEd,EM_REPLACESEL,TRUE,rbuf);cnt++;}
  SM(hEd,WM_SETREDRAW,1,0);InvalidateRect(hEd,0,1);
  wchar_t s[64];_snwprintf(s,64,L"Replaced %d occurrence(s).",cnt);MessageBoxW(hFind?hFind:hWnd,s,APP,MB_ICONINFORMATION);return;}
 CHARRANGE c;SM(hEd,EM_EXGETSEL,0,&c);int L=(int)wcslen(fbuf);
 if(c.cpMax-c.cpMin==L&&L<256){wchar_t s[300];SM(hEd,EM_GETSELTEXT,0,s);
  if(!((fl&FR_MATCHCASE)?wcscmp(s,fbuf):_wcsicmp(s,fbuf)))SM(hEd,EM_REPLACESEL,TRUE,rbuf);}
 dofind(fl,1);}
static void openfind(int rep){
 if(hFind)DestroyWindow(hFind);
 CHARRANGE c;SM(hEd,EM_EXGETSEL,0,&c);if(c.cpMax>c.cpMin&&c.cpMax-c.cpMin<200)SM(hEd,EM_GETSELTEXT,0,fbuf);
 DWORD keep=fr.Flags&(FR_MATCHCASE|FR_WHOLEWORD);memset(&fr,0,sizeof fr);fr.lStructSize=sizeof fr;fr.hwndOwner=hWnd;
 fr.lpstrFindWhat=fbuf;fr.wFindWhatLen=256;fr.lpstrReplaceWith=rbuf;fr.wReplaceWithLen=256;fr.Flags=FR_DOWN|keep;
 hFind=rep?ReplaceTextW(&fr):FindTextW(&fr);}

/* ---------- commands ---------- */
static void cmd(int id){
 HWND f=GetFocus()==hPv?hPv:hEd;
 if(id>=ID_FONT0&&id<ID_FONT0+NF){wcscpy(fontName,FONTS[id-ID_FONT0]);style();return;}
 if(id>=ID_SIZE0&&id<ID_SIZE0+NS){fontPt=SIZES[id-ID_SIZE0];style();return;}
 if(id>=ID_THEME0&&id<ID_THEME0+NT){theme=id-ID_THEME0;style();return;}
 if(id>=ID_H1&&id<=ID_H6){blocks(id-ID_H1+1);return;}
 switch(id){
 case ID_NEW:newdoc();break;
 case ID_OPEN:openfile();break;
 case ID_SAVE:save(0);break;
 case ID_SAVEAS:save(1);break;
 case ID_CLOSE:closedoc(cur);break;
 case ID_SAVEALL:for(int i=0;i<nd;i++)if(moded(i)){switchdoc(i);save(0);}break;
 case ID_NEXT:switchdoc((cur+1)%nd);break;
 case ID_PREV:switchdoc((cur+nd-1)%nd);break;
 case ID_EXIT:SM(hWnd,WM_CLOSE,0,0);break;
 case ID_UNDO:SM(hEd,EM_UNDO,0,0);break;
 case ID_REDO:SM(hEd,EM_REDO,0,0);break;
 case ID_CUT:SM(f,WM_CUT,0,0);break;
 case ID_COPY:SM(f,WM_COPY,0,0);break;
 case ID_PASTE:SM(hEd,WM_PASTE,0,0);break;
 case ID_DELETE:SM(hEd,WM_CLEAR,0,0);break;
 case ID_SELALL:SM(f,EM_SETSEL,0,-1);break;
 case ID_FIND:openfind(0);break;
 case ID_REPLACE:openfind(1);break;
 case ID_FINDNEXT:if(fbuf[0])dofind(fr.Flags|FR_DOWN,1);else openfind(0);break;
 case ID_NORMAL:blocks(0);break;case ID_QUOTE:blocks(10);break;case ID_BULLET:blocks(11);break;
 case ID_NUMBER:blocks(12);break;case ID_TASK:blocks(13);break;case ID_INDENT:blocks(20);break;case ID_OUTDENT:blocks(21);break;
 case ID_HR:SM(hEd,EM_REPLACESEL,TRUE,L"\r\r---\r\r");break;
 case ID_BOLD:wrapsel(L"**",L"**");break;case ID_ITALIC:wrapsel(L"*",L"*");break;case ID_STRIKE:wrapsel(L"~~",L"~~");break;
 case ID_CODE:wrapsel(L"`",L"`");break;case ID_CODEBLK:wrapsel(L"```\r",L"\r```");break;
 case ID_LINK:wrapsel(L"[",L"](https://)");break;case ID_IMAGE:wrapsel(L"![",L"](image.png)");break;
 case ID_FONTMORE:{LOGFONTW lf;memset(&lf,0,sizeof lf);lf.lfHeight=-MulDiv(fontPt,dpi,72);wcscpy(lf.lfFaceName,fontName);
  CHOOSEFONTW cf;memset(&cf,0,sizeof cf);cf.lStructSize=sizeof cf;cf.hwndOwner=hWnd;cf.lpLogFont=&lf;
  cf.Flags=CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT|CF_FORCEFONTEXIST|CF_LIMITSIZE|CF_NOSCRIPTSEL;cf.nSizeMin=6;cf.nSizeMax=48;
  if(ChooseFontW(&cf)){wcscpy(fontName,lf.lfFaceName);fontPt=cf.iPointSize/10;style();}break;}
 case ID_VEDIT:case ID_VSPLIT:case ID_VPREV:mode=id-ID_VEDIT;sync();layout();if(mode)refresh_preview();SetFocus(mode==2?hPv:hEd);break;
 case ID_WRAP:wrap=!wrap;for(int i=0;i<nd;i++)SM(docs[i]->ed,EM_SETTARGETDEVICE,0,wrap?0:1);sync();break;
 case ID_SBAR:showSb=!showSb;sync();layout();break;
 case ID_TOP:top=!top;SetWindowPos(hWnd,top?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);sync();break;
 case ID_ZIN:zoom(10);break;case ID_ZOUT:zoom(-10);break;case ID_ZRESET:zoom(0);break;}}

static void mkmenu(void){
 HMENU f=CreatePopupMenu(),e=CreatePopupMenu(),p=CreatePopupMenu(),o=CreatePopupMenu(),t;
 mBar=CreateMenu();mView=CreatePopupMenu();mTheme=t=CreatePopupMenu();mFont=CreatePopupMenu();mSize=CreatePopupMenu();
#define A(h,i,s) AppendMenuW(h,MF_STRING,i,s)
#define S(h) AppendMenuW(h,MF_SEPARATOR,0,0)
 A(f,ID_NEW,L"&New Tab\tCtrl+N");A(f,ID_OPEN,L"&Open...\tCtrl+O");A(f,ID_CLOSE,L"&Close Tab\tCtrl+W");S(f);
 A(f,ID_SAVE,L"&Save\tCtrl+S");A(f,ID_SAVEAS,L"Save &As...\tCtrl+Shift+S");A(f,ID_SAVEALL,L"Save A&ll");S(f);
 A(f,ID_NEXT,L"N&ext Tab\tCtrl+Tab");A(f,ID_PREV,L"Pre&vious Tab\tCtrl+Shift+Tab");S(f);A(f,ID_EXIT,L"E&xit");
 A(e,ID_UNDO,L"&Undo\tCtrl+Z");A(e,ID_REDO,L"&Redo\tCtrl+Y");S(e);A(e,ID_CUT,L"Cu&t\tCtrl+X");A(e,ID_COPY,L"&Copy\tCtrl+C");
 A(e,ID_PASTE,L"&Paste\tCtrl+V");A(e,ID_DELETE,L"&Delete\tDel");A(e,ID_SELALL,L"Select &All\tCtrl+A");S(e);
 A(e,ID_FIND,L"&Find...\tCtrl+F");A(e,ID_FINDNEXT,L"Find &Next\tF3");A(e,ID_REPLACE,L"&Replace...\tCtrl+H");
 for(int i=0;i<6;i++){wchar_t s[32];_snwprintf(s,32,L"Heading &%d\tCtrl+%d",i+1,i+1);A(p,ID_H1+i,s);}
 A(p,ID_NORMAL,L"&Normal Paragraph");S(p);A(p,ID_QUOTE,L"&Quote");A(p,ID_BULLET,L"&Bullet List");A(p,ID_NUMBER,L"N&umbered List");
 A(p,ID_TASK,L"&Task List");S(p);A(p,ID_INDENT,L"&Indent");A(p,ID_OUTDENT,L"&Outdent");S(p);A(p,ID_HR,L"Horizontal &Rule");
 A(o,ID_BOLD,L"&Bold\tCtrl+B");A(o,ID_ITALIC,L"&Italic\tCtrl+I");A(o,ID_STRIKE,L"&Strikethrough");A(o,ID_CODE,L"Inline &Code\tCtrl+`");
 A(o,ID_CODEBLK,L"Code &Block");S(o);A(o,ID_LINK,L"&Link\tCtrl+K");A(o,ID_IMAGE,L"I&mage");S(o);
 for(int i=0;i<NF;i++)A(mFont,ID_FONT0+i,FONTS[i]);S(mFont);A(mFont,ID_FONTMORE,L"More fonts...");
 for(int i=0;i<NS;i++){wchar_t s[16];_snwprintf(s,16,L"%d pt",SIZES[i]);A(mSize,ID_SIZE0+i,s);}
 AppendMenuW(o,MF_POPUP,(UINT_PTR)mFont,L"&Font");AppendMenuW(o,MF_POPUP,(UINT_PTR)mSize,L"Font &Size");
 A(mView,ID_VEDIT,L"&Editor Only\tF5");A(mView,ID_VSPLIT,L"&Split View\tF6");A(mView,ID_VPREV,L"&Reading View\tF7");S(mView);
 A(mView,ID_WRAP,L"&Word Wrap");A(mView,ID_SBAR,L"Status &Bar");A(mView,ID_TOP,L"Always on &Top");S(mView);
 A(mView,ID_ZIN,L"Zoom &In\tCtrl++");A(mView,ID_ZOUT,L"Zoom &Out\tCtrl+-");A(mView,ID_ZRESET,L"&Reset Zoom\tCtrl+0");
 for(int i=0;i<NT;i++)A(t,ID_THEME0+i,TH[i].n);
 AppendMenuW(mBar,MF_POPUP,(UINT_PTR)f,L"&File");AppendMenuW(mBar,MF_POPUP,(UINT_PTR)e,L"&Edit");
 AppendMenuW(mBar,MF_POPUP,(UINT_PTR)p,L"&Paragraph");AppendMenuW(mBar,MF_POPUP,(UINT_PTR)o,L"F&ormat");
 AppendMenuW(mBar,MF_POPUP,(UINT_PTR)mView,L"&View");AppendMenuW(mBar,MF_POPUP,(UINT_PTR)t,L"&Theme");}

static LRESULT CALLBACK WP(HWND h,UINT m,WPARAM w,LPARAM l){
 if(WM_FINDMSG&&m==WM_FINDMSG){FINDREPLACEW*q=(FINDREPLACEW*)l;
  if(q->Flags&FR_DIALOGTERM)hFind=0;
  else if(q->Flags&FR_FINDNEXT)dofind(q->Flags,1);
  else if(q->Flags&FR_REPLACE)doreplace(q->Flags,0);
  else if(q->Flags&FR_REPLACEALL)doreplace(q->Flags,1);
  return 0;}
 switch(m){
 case WM_CREATE:{hWnd=h;int mg=12*dpi/96;
  hTab=CreateWindowExW(0,L"MdTabs",L"",WS_CHILD|WS_VISIBLE,0,0,0,0,h,(HMENU)4,hInst,0);
  hPv=CreateWindowExW(0,L"RICHEDIT50W",L"",WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY,0,0,0,0,h,(HMENU)2,hInst,0);
  hSb=CreateWindowExW(0,STATUSCLASSNAMEW,L"",WS_CHILD|WS_VISIBLE,0,0,0,0,h,(HMENU)3,hInst,0);
  SM(hPv,EM_SETUNDOLIMIT,0,0);SetWindowSubclass(hPv,EP,1,0);SM(hPv,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,MAKELONG(mg,mg));
  DragAcceptFiles(h,1);return 0;}
 case WM_SIZE:layout();return 0;
 case WM_SETFOCUS:if(hEd)SetFocus(mode==2?hPv:hEd);return 0;
 case WM_ERASEBKGND:{RECT r;GetClientRect(h,&r);HBRUSH b=CreateSolidBrush(TH[theme].cb);FillRect((HDC)w,&r,b);DeleteObject(b);return 1;}
 /* draggable splitter between editor and preview */
 case WM_SETCURSOR:
  if(mode==1&&(HWND)w==h){POINT pt;GetCursorPos(&pt);ScreenToClient(h,&pt);
   if(pt.x>=gapX&&pt.x<gapX+gapW&&pt.y>=tabH){SetCursor(LoadCursor(0,IDC_SIZEWE));return TRUE;}}
  break;
 case WM_LBUTTONDOWN:{int x=(short)LOWORD(l),y=(short)HIWORD(l);
  if(mode==1&&x>=gapX&&x<gapX+gapW&&y>=tabH){drag=1;SetCapture(h);}return 0;}
 case WM_LBUTTONDBLCLK:{int x=(short)LOWORD(l);if(mode==1&&x>=gapX&&x<gapX+gapW){splitPct=50;layout();}return 0;}
 case WM_MOUSEMOVE:
  if(drag){RECT r;GetClientRect(h,&r);if(r.right>gapW){int p=((short)LOWORD(l)-gapW/2)*100/(r.right-gapW);
   if(p<15)p=15;if(p>85)p=85;if(p!=splitPct){splitPct=p;layout();}}}
  return 0;
 case WM_LBUTTONUP:if(drag){drag=0;ReleaseCapture();}return 0;
 case WM_COMMAND:
  if(LOWORD(w)==1&&HIWORD(w)==EN_CHANGE&&l){pvDirty=1;KICK;}else if(!l)cmd(LOWORD(w));return 0;
 case WM_NOTIFY:{NMHDR*nh=(NMHDR*)l;
  if(nh->idFrom==1){if(nh->code==EN_SELCHANGE)KICK;
   else if(nh->code==EN_DROPFILES){dropfiles(((ENDROPFILES*)l)->hDrop);return 1;}}
  return 0;}
 case WM_TIMER:KillTimer(h,1);title();status();if(pvDirty)refresh_preview();return 0;
 case WM_DRAWITEM:{DRAWITEMSTRUCT*d=(DRAWITEMSTRUCT*)l;
  if(d->hwndItem==hSb){HBRUSH b=CreateSolidBrush(TH[theme].bg);FillRect(d->hDC,&d->rcItem,b);DeleteObject(b);
   SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,TH[theme].fg);RECT r=d->rcItem;r.left+=8;
   DrawTextW(d->hDC,(wchar_t*)d->itemData,-1,&r,DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);return TRUE;}
  break;}
 case WM_DROPFILES:dropfiles((HDROP)w);DragFinish((HDROP)w);return 0;
 case WM_CLOSE:if(ask_all()){saveini();DestroyWindow(h);}return 0;
 case WM_DESTROY:PostQuitMessage(0);return 0;}
 return DefWindowProcW(h,m,w,l);}

static ACCEL AC[]={
 {FVIRTKEY|FCONTROL,'N',ID_NEW},{FVIRTKEY|FCONTROL,'O',ID_OPEN},{FVIRTKEY|FCONTROL,'W',ID_CLOSE},
 {FVIRTKEY|FCONTROL,'S',ID_SAVE},{FVIRTKEY|FCONTROL|FSHIFT,'S',ID_SAVEAS},
 {FVIRTKEY|FCONTROL,VK_TAB,ID_NEXT},{FVIRTKEY|FCONTROL|FSHIFT,VK_TAB,ID_PREV},
 {FVIRTKEY|FCONTROL,'F',ID_FIND},{FVIRTKEY|FCONTROL,'H',ID_REPLACE},{FVIRTKEY,VK_F3,ID_FINDNEXT},
 {FVIRTKEY|FCONTROL,'B',ID_BOLD},{FVIRTKEY|FCONTROL,'I',ID_ITALIC},{FVIRTKEY|FCONTROL,'K',ID_LINK},{FVIRTKEY|FCONTROL,VK_OEM_3,ID_CODE},
 {FVIRTKEY|FCONTROL,'1',ID_H1},{FVIRTKEY|FCONTROL,'2',ID_H2},{FVIRTKEY|FCONTROL,'3',ID_H3},
 {FVIRTKEY|FCONTROL,'4',ID_H4},{FVIRTKEY|FCONTROL,'5',ID_H5},{FVIRTKEY|FCONTROL,'6',ID_H6},
 {FVIRTKEY,VK_F5,ID_VEDIT},{FVIRTKEY,VK_F6,ID_VSPLIT},{FVIRTKEY,VK_F7,ID_VPREV},
 {FVIRTKEY|FCONTROL,VK_OEM_PLUS,ID_ZIN},{FVIRTKEY|FCONTROL,VK_OEM_MINUS,ID_ZOUT},{FVIRTKEY|FCONTROL,'0',ID_ZRESET}};

int WINAPI wWinMain(HINSTANCE hi,HINSTANCE hp,PWSTR cl,int sw){
 (void)hp;(void)cl;hInst=hi;LoadLibraryW(L"Msftedit.dll");
 INITCOMMONCONTROLSEX ic={sizeof ic,ICC_BAR_CLASSES};InitCommonControlsEx(&ic);
 loadini();WM_FINDMSG=RegisterWindowMessageW(FINDMSGSTRINGW);
 HDC dc=GetDC(0);dpi=GetDeviceCaps(dc,LOGPIXELSX);ReleaseDC(0,dc);
 tabFont=CreateFontW(-MulDiv(9,dpi,72),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 WNDCLASSEXW wc;memset(&wc,0,sizeof wc);wc.cbSize=sizeof wc;wc.style=CS_DBLCLKS;wc.lpfnWndProc=WP;wc.hInstance=hi;
 wc.hCursor=LoadCursor(0,IDC_ARROW);wc.hIcon=LoadIconW(hi,MAKEINTRESOURCEW(1));wc.hIconSm=(HICON)LoadImageW(hi,MAKEINTRESOURCEW(1),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0);wc.lpszClassName=L"AnnoMD";RegisterClassExW(&wc);
 WNDCLASSEXW tc;memset(&tc,0,sizeof tc);tc.cbSize=sizeof tc;tc.lpfnWndProc=TP;tc.hInstance=hi;
 tc.hCursor=LoadCursor(0,IDC_ARROW);tc.lpszClassName=L"MdTabs";RegisterClassExW(&tc);
 mkmenu();
 HWND h=CreateWindowExW(0,L"AnnoMD",APP,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,CW_USEDEFAULT,CW_USEDEFAULT,1100*dpi/96,720*dpi/96,0,mBar,hi,0);
 if(!h)return 1;
 if(top)SetWindowPos(h,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
 newdoc();style();layout();ShowWindow(h,sw);UpdateWindow(h);
 int ac;wchar_t**av=CommandLineToArgvW(GetCommandLineW(),&ac);for(int k=1;av&&k<ac;k++)openpath(av[k]);
 HACCEL ha=CreateAcceleratorTableW(AC,sizeof AC/sizeof AC[0]);MSG msg;
 while(GetMessageW(&msg,0,0,0)){
  if(hFind&&IsDialogMessageW(hFind,&msg))continue;
  if(TranslateAcceleratorW(h,ha,&msg))continue;
  TranslateMessage(&msg);DispatchMessageW(&msg);}
 return (int)msg.wParam;}
