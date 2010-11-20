/**************************************************

The plot.h include file allows to directly output
graphics / perform simple drawings in a standard
Windows 95/NT window

More information at http://club.ib.be/plot.htm

***************************************************/

#include <windows.h>
#include <fstream.h>
#include <stdio.h>
#include <crtdbg.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <strstrea.h>
#include <typeinfo.h>


/* GWF - removing all references to BITMAPWIDTH and BITMAPHEIGHT.
   instead, I am going to have the width and height passed to the
   applications through environment variables.  These vaules will
   be contained in win98width and win98height.  Note that this is
   not the values that the rest of the programs see.  If they want
   a smaller size, then the subplot will be centered in the main
   canvas.  If they want somthing larger, then it will be linearly
   rescaled by the code in winplot.cpp.

   Also, all references to B are now done with pointers, since this
   is now dynamically allocated. */


static int win98width = 640, win98height = 480;

// 64 bit integer type :
typedef __int64 malabar;
#define MALABAR(a,b) (((malabar) a) << 32) + (malabar) b
#define LOW32(a) (long)a
#define HIGH32(a) (long)(a >> 32)

malabar MilliTime ()
{
   malabar Time;
   static malabar PreviousTime = 0;
   static malabar Turns = 0;

   SYSTEMTIME t;
   GetSystemTime (&t);

   Time = 
         (malabar) t.wMilliseconds + 
         (malabar) t.wSecond * 1000 +
         (malabar) t.wMinute * 60000 +
         (malabar) t.wHour * 3600000 +
         (malabar) t.wDayOfWeek * 86400000;
   
   if (Time < PreviousTime) Turns++;
   PreviousTime = Time;

   Time += Turns * 604800000;

   return Time;
}

double Rnd ()
{
   static BOOL first = TRUE;
   if (first) 
   {
      srand ((int) MilliTime());
      first = FALSE;
   }
   return (double) rand() / ((double) RAND_MAX + 1);
}

#define CR  '\x0D'   // 13
#define LF  '\x0A'   // 10
#define BS  '\x08'   //  8
#define TAB '\x09'   //  9
#define FF  '\x0C'   // 12
#define CU  '\x0E'   // 14
#define CD  '\x0F'   // 15
#define CL  '\x10'   // 16
#define CI  '\x11'   // 17

char Inkey ();
void Refresh ();

class Bitmap 
{
public :

   BITMAPINFO           BitmapInfo;
   unsigned char*       BitmapArray;
   HDC                  hDC;
   HBITMAP              hBitmap;
   HBITMAP              hBitmapOld;
   RECT                 Rect;          // Area that must be redrawn because of changes
   int                  Width;
   int                  Height;
   int                  LineWidth;
   BYTE                 InkR;
   BYTE                 InkG;
   BYTE                 InkB;
   BYTE                 PaperR;
   BYTE                 PaperG;
   BYTE                 PaperB;
   int                  LocateL;
   int                  LocateC;
   double               TurtleX;
   double               TurtleY;
   double               TurtleDirection;
   double               Pi;
   BOOL                 TurtlePenTracing;
   int                  CharacterWidth;
   int                  CharacterHeight;
   int                  Lines;
   int                  Columns;


   void CreateDIBDC ()
   {
      HDC hdc1;

      hdc1 = CreateDC ("DISPLAY", NULL, NULL, NULL);
      hDC = CreateCompatibleDC (hdc1);

      DeleteDC (hdc1);
   }

   void DeleteDIBDC ()
   {
      DeleteDC (hDC);
   }

   Bitmap (int w, int h)
   {
      Width = w;
      Height = h;
      LineWidth = 0;

      // GWF - inverted ink and paper colors
      InkR = 255;
      InkG = 255;
      InkB = 255;
      PaperR = 0;
      PaperG = 0;
      PaperB = 0;

      LocateL = 1;
      LocateC = 1;
      TurtleX = w / 2;
      TurtleY = h / 2;
      Pi = atan (1) * 4;
      TurtleDirection = Pi / 2;
      TurtlePenTracing = TRUE;
      CharacterWidth = 8;
      CharacterHeight = 15;
      Lines = Height / CharacterHeight;
      Columns = Width / CharacterWidth;

      // Initialisation du DIB
      BitmapInfo.bmiHeader.biSize =          sizeof(BITMAPINFOHEADER);
      BitmapInfo.bmiHeader.biWidth =         w;
      BitmapInfo.bmiHeader.biHeight =        h;
      BitmapInfo.bmiHeader.biPlanes =        1;
      BitmapInfo.bmiHeader.biBitCount =      24;
      BitmapInfo.bmiHeader.biCompression =   BI_RGB;
      BitmapInfo.bmiHeader.biSizeImage =     w * h * 3;
      BitmapInfo.bmiHeader.biXPelsPerMeter = 0;
      BitmapInfo.bmiHeader.biYPelsPerMeter = 0;
      BitmapInfo.bmiHeader.biClrUsed =       0;
      BitmapInfo.bmiHeader.biClrImportant =  0;
   
      // BitmapArray = new unsigned char[Width * Height * 3];

      BitmapArray = NULL;

      hBitmap = CreateDIBSection (NULL,
                              &BitmapInfo,
                              DIB_RGB_COLORS,
                              (void **) &BitmapArray,
                              NULL, 
                              0);

      if (BitmapArray == NULL) _RPT0 (_CRT_ERROR, "Not enough memory for BitmapArray");
      if (hBitmap == NULL) _RPT0 (_CRT_ERROR, "Could not create Bitmap");

      CreateDIBDC ();

      hBitmapOld = SelectObject (hDC, hBitmap);

   }

   ~Bitmap()
   {
      SelectObject (hDC, hBitmapOld);
      DeleteDIBDC ();
      DeleteObject (hBitmap);
   }

   void Ink (BYTE r, BYTE g, BYTE b)
   {
      InkR = r;
      InkG = g;
      InkB = b;
   }

   void Paper (BYTE r, BYTE g, BYTE b)
   {
      PaperR = r;
      PaperG = g;
      PaperB = b;
   }

   void Plot (int x, int y, BYTE r, BYTE g, BYTE b)
   {
      long a, oy;

      if ((x >= 0) && (x < Width) && (y >= 0) && y < Height)
      {
         a = (x + Width * y) * 3;
         // if (IsBadWritePtr (BitmapArray, 4)) _RPT0 (_CRT_ERROR, "No access !");

         BitmapArray[a + 2] = r;
         BitmapArray[a + 1] = g;
         BitmapArray[a + 0] = b;

         if (Rect.left > x) Rect.left = x;
         if (Rect.right <= x) Rect.right = x + 2;

         oy = Height - y - 1;
         if (Rect.top > oy) Rect.top = oy;
         if (Rect.bottom <= oy) Rect.bottom = oy + 2;
      }
   }

   void Plot (int x, int y)
   {
      Plot (x, y, InkR, InkG, InkB);
   }

   void Pixel (int x, int y, BYTE &r, BYTE &g, BYTE &b)
   {
      long a;
  
      if ((x >= 0) && (x < Width) && (y >= 0) && y < Height)
      {
         a = (x + Width * y) * 3;
         r = BitmapArray[a + 2];
         g = BitmapArray[a + 1];
         b = BitmapArray[a + 0];
      }
      else
      {
         r = PaperR;
         g = PaperG;
         b = PaperB;
      }
   }

   BYTE PixelR (int x, int y)
   {
      long a;
      BYTE r;
  
      if ((x >= 0) && (x < Width) && (y >= 0) && y < Height)
      {
         a = (x + Width * y) * 3;
         r = BitmapArray[a + 2];
      }
      else r = PaperR;
      return r;
   }

   BYTE PixelG (int x, int y)
   {
      long a;
      BYTE g;
  
      if ((x >= 0) && (x < Width) && (y >= 0) && y < Height)
      {
         a = (x + Width * y) * 3;
         g = BitmapArray[a + 1];
      }
      else g = PaperG;
      return g;
   }

   BYTE PixelB (int x, int y)
   {
      long a;
      BYTE b;
  
      if ((x >= 0) && (x < Width) && (y >= 0) && y < Height)
      {
         a = (x + Width * y) * 3;
         b = BitmapArray[a + 0];
      }
      else b = PaperB;
      return b;
   }

   void Print (int x, int y, const char* s)
   {
      int height, oy, len, ol;
      RECT r;

      len = strlen (s);
   
      r.top = Height - y - 1; 
      r.left = x; 
      r.bottom = Height; 
      r.right = Width;

      HFONT hFont, hFontOld;
      hFont = CreateFont( 0, 0,
                          0, 0,
                          FW_NORMAL,
                          FALSE,
                          FALSE,
                          FALSE,
                          ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS,
                          CLIP_DEFAULT_PRECIS,
                          DEFAULT_QUALITY,
                          FIXED_PITCH || FF_DONTCARE,
                          "fixedsys");

      if (hFont == NULL) _RPT0 (_CRT_ERROR, "Could not create Font");

      hFontOld = SelectObject (hDC, hFont);

      SetTextColor (hDC, RGB (InkR, InkG, InkB));
      SetBkColor (hDC, RGB (PaperR, PaperG, PaperB));

      TextOut (hDC, x, Height - y - 1, s, len);

      SelectObject (hDC, hFontOld);
      DeleteObject (hFont);


      height = 20;

      if (Rect.left > x) Rect.left = x;
      ol = x + len * CharacterWidth + 2;
      if (Rect.right < ol) Rect.right = ol;

      oy = Height - y - 1;
      if (Rect.top > oy) Rect.top = oy;
      ol = oy + CharacterHeight + 2;
      if (Rect.bottom < ol) Rect.bottom = ol;
   }

   void FontPrint (int x, int y, char* s, char* fn, int h, BOOL b, BOOL i, BOOL u)
   {
      int height, oy, len, ol;
      RECT r;

      len = strlen (s);
   
      r.top = Height - y - 1; 
      r.left = x; 
      r.bottom = Height; 
      r.right = Width;


      HFONT hFont, hFontOld;
      hFont = CreateFont( h, 0,
                          0, 0,
                          b ? FW_BOLD : FW_NORMAL,
                          i,
                          u,
                          FALSE,
                          ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS,
                          CLIP_DEFAULT_PRECIS,
                          DEFAULT_QUALITY,
                          DEFAULT_PITCH || FF_DONTCARE,
                          fn);

      if (hFont == NULL) _RPT0 (_CRT_ERROR, "Could not create Font");

      hFontOld = SelectObject (hDC, hFont);

      SetTextColor (hDC, RGB (InkR, InkG, InkB));
      SetBkColor (hDC, RGB (PaperR, PaperG, PaperB));

      //height = DrawText (hDC, s, strlen (s), &r, DT_LEFT | DT_TOP);
      TextOut (hDC, x, Height - y - 1, s, len);

      SelectObject (hDC, hFontOld);
      DeleteObject (hFont);


      height = 20;

      if (Rect.left > x) Rect.left = x;
      ol = Width + 2;
      if (Rect.right < ol) Rect.right = ol;

      oy = Height - y - 1;
      if (Rect.top > oy) Rect.top = oy;
      ol = oy + h * 3 + 2;
      if (Rect.bottom < ol) Rect.bottom = ol;
   }

   void Locate (int l, int c)
   {
      LocateL = l;
      LocateC = c;
      if (LocateL < 1) LocateL = 1;
      if (LocateL > Lines) LocateL = Lines;
      if (LocateC < 1) LocateC = 1;
      if (LocateC > Columns) LocateC = Columns;
   }

   void Print (char *s)
   {
      char *t = s;
      char c;
      char d[1024];
      int i;
label:
      if (strlen (t) == 0) return;
      if (t[0] < 32 && t[0] >= 0)
      {
         c = t[0];
         t++;
         switch (c)
         {
         case BS:
            LocateC--;
            Print (" ");
            LocateC--;
            break;
         case CR:
            LocateC = 1;
            break;
         case LF:
            LocateC = 1;
            LocateL++;
            break;
         case TAB:
            LocateC += 12;
            LocateC = 12 * (LocateC / 12);
            break;
         case FF:
            Cls();
            break;
         case CU:
            LocateL--;
            break;
         case CD:
            LocateL++;
            break;
         case CL:
            LocateC--;
            break;
         case CI:
            LocateC++;
            break;
         }
         goto label;
      }

      if (LocateC > Columns)
      {
         i = ((LocateC - 1) / Columns);
         LocateC -= Columns * i;
         LocateL += i;
      }
      if (LocateC < 1)
      {
         i = ((Columns - LocateC) / Columns);
         LocateC += Columns * i;
         LocateL -= i;
      }
      if (LocateL > Lines)
      {
         Scroll ((LocateL - Lines) * CharacterHeight);
         LocateL = Lines;
      }
      if (LocateL < 1)
      {
         Scroll ((1 - LocateL) * CharacterHeight);
         LocateL = 1;
      }

      for (i = 0; ; i++)
      {
         if ((t[i] < 32 && t[i] >= 0) || i + LocateC > Columns) break;
         d[i] = t[i];
         d[i + 1] = 0;
      }

      Print ((LocateC - 1) * CharacterWidth, 
             (Lines - LocateL + 1) * CharacterHeight, 
             d);
      LocateC += i;
      t += i;

      goto label;
   }

   void Input (char *s, int lmax)
   {
      char c;
      int l = 0;
      s[0] = 0;
      Print ("_"); Refresh(); 
      do
      {
         c = Inkey();
         if (c != CR)
         {
            if (c == BS)
            {
               if (l > 0)
               {
                  l--;
                  s[l] = 0;
                  Print ("\x08\x08_"); Refresh();
               }
            }
            else
            {
               if ((c >= 32 || c < 0) && l < lmax - 1)
               {
                  s[l] = c;
                  l++;
                  s[l] = 0;
                  Print ("\x08");
                  Print (&(s[l-1]));
                  Print ("_"); Refresh();
               }
            }
         }
      } while (c != CR);
      Print ("\x08"); Refresh();
   }

   void Line (int x1, int y1, int x2, int y2, BYTE r = 0, BYTE g = 0, BYTE b = 0)
   {
      HPEN hPen, hPenOld;
      int oy1, oy2;

      oy1 = Height - y1 - 1;
      oy2 = Height - y2 - 1;

      hPen = CreatePen (PS_SOLID, LineWidth, RGB(r, g, b));
      hPenOld = SelectObject (hDC, hPen);

      MoveToEx (hDC, x1, oy1, NULL);
      LineTo (hDC, x2, oy2);

      SetPixel (hDC, x2, oy2, RGB(r, g, b));

      SelectObject (hDC, hPenOld);
      DeleteObject (hPen);

      if (Rect.left > x1 - LineWidth) Rect.left = x1 - LineWidth;
      if (Rect.right <= x1 + LineWidth) Rect.right = x1 + 2 + LineWidth;

      if (Rect.top > oy1 - LineWidth) Rect.top = oy1 - LineWidth;
      if (Rect.bottom <= oy1 + LineWidth) Rect.bottom = oy1 + 2 + LineWidth;

      if (Rect.left > x2 - LineWidth) Rect.left = x2 - LineWidth;
      if (Rect.right <= x2 + LineWidth) Rect.right = x2 + 2 + LineWidth;

      if (Rect.top > oy2 - LineWidth) Rect.top = oy2 - LineWidth;
      if (Rect.bottom <= oy2 + LineWidth) Rect.bottom = oy2 + 2 + LineWidth;

   }

   void Circle (int xc, int yc, int ra, BYTE r = 0, BYTE g = 0, BYTE b = 0)
   {
      HPEN hPen, hPenOld;
      int oy1, oy2;
      int x1, y1, x2, y2;

      x1 = xc - ra;
      y1 = yc + ra;

      x2 = xc + ra;
      y2 = yc - ra;

      oy1 = Height - y1 - 1;
      oy2 = Height - y2 - 1;

      hPen = CreatePen (PS_SOLID, LineWidth, RGB(r, g, b));
      hPenOld = SelectObject (hDC, hPen);

      Arc (hDC, x1, oy1, x2, oy2, 0,0,0,0); // x1, (oy1 + oy2) / 2, x1, (oy1 + oy2) / 2);

      SelectObject (hDC, hPenOld);
      DeleteObject (hPen);


      if (Rect.left > x1 - LineWidth) Rect.left = x1 - LineWidth;
      if (Rect.right <= x1 + LineWidth) Rect.right = x1 + 2 + LineWidth;

      if (Rect.top > oy1 - LineWidth) Rect.top = oy1 - LineWidth;
      if (Rect.bottom <= oy1 + LineWidth) Rect.bottom = oy1 + 2 + LineWidth;

      if (Rect.left > x2 - LineWidth) Rect.left = x2 - LineWidth;
      if (Rect.right <= x2 + LineWidth) Rect.right = x2 + 2 + LineWidth;

      if (Rect.top > oy2 - LineWidth) Rect.top = oy2 - LineWidth;
      if (Rect.bottom <= oy2 + LineWidth) Rect.bottom = oy2 + 2 + LineWidth;
   }

   void Cls (BYTE r = 255, BYTE g = 255, BYTE b = 255)
   {
      long i, l;
      unsigned long b1, b2, b3;
      unsigned long *p;
      p = (unsigned long*) BitmapArray;

      b1 = b;
      b1 = (b1 << 8) + r;
      b1 = (b1 << 8) + g;
      b1 = (b1 << 8) + b;

      b2 = g;
      b2 = (b2 << 8) + b;
      b2 = (b2 << 8) + r;
      b2 = (b2 << 8) + g;

      b3 = r;
      b3 = (b3 << 8) + g;
      b3 = (b3 << 8) + b;
      b3 = (b3 << 8) + r;

      l = Width * Height * 3 / 4;
   
      for (i = 0; i < l;) 
      {
         p [i++] = b1;
         p [i++] = b2;
         p [i++] = b3;
      }

      LocateL = 1;
      LocateC = 1;

      Rect.left = 0;
      Rect.top = 0;
      Rect.right = Width;
      Rect.bottom = Height;
   }

   void Scroll (int a, BYTE r = 255, BYTE g = 255, BYTE b = 255)
   {
      int i, l, l1, l2;
      unsigned long b1, b2, b3;
      unsigned long *p;
      p = (unsigned long*) BitmapArray;

      if (a == 0) return;
      if (abs(a) >= Height) Cls(r, g, b);
      else
      {
         if (a > 0) 
         {
            MoveMemory (&(BitmapArray[a*Width*3]), BitmapArray, (Height-a)*Width*3);
            l1 = 0;
            l2 = a;
         }
         else
         {
            MoveMemory (BitmapArray, &(BitmapArray[-a*Width*3]), (Height+a)*Width*3);
            l1 = Height - 1 + a;
            l2 = Height - 1;
         }

         b1 = b;
         b1 = (b1 << 8) + r;
         b1 = (b1 << 8) + g;
         b1 = (b1 << 8) + b;

         b2 = g;
         b2 = (b2 << 8) + b;
         b2 = (b2 << 8) + r;
         b2 = (b2 << 8) + g;

         b3 = r;
         b3 = (b3 << 8) + g;
         b3 = (b3 << 8) + b;
         b3 = (b3 << 8) + r;

         l = Width * l2 * 3 / 4;
   
         for (i = Width * l1 * 3 / 4; i < l;) 
         {
            p [i++] = b1;
            p [i++] = b2;
            p [i++] = b3;
         }

         Rect.left = 0;
         Rect.top = 0;
         Rect.right = Width;
         Rect.bottom = Height;
      }
   }

   void Pann (int a, BYTE r = 255, BYTE g = 255, BYTE b = 255)
   {
      int i, j, w;

      if (a == 0) return;
      if (abs(a) >= Width) Cls(r, g, b);
      else
      {
         //SetThreadPriority (MyThread, THREAD_PRIORITY_HIGHEST);
         w = Width - abs(a); 
         if (a > 0) 
         {
            for (i = 0; i < Height; i++)
            {
               MoveMemory (&(BitmapArray[(i*Width+a)*3]), &(BitmapArray[i*Width*3]), w * 3);
               if (i == 0) for(j = 0; j < a; j++) Plot (j, i, r, g, b);
               else MoveMemory (&(BitmapArray[i*Width*3]), BitmapArray, a * 3);
            }
         }

         else

         {
            for (i = 0; i < Height; i++)
            {
               MoveMemory (&(BitmapArray[i*Width*3]), &(BitmapArray[(i*Width-a)*3]), w * 3);
               if (i == 0) for(j = w; j < Width; j++) Plot (j, i, r, g, b);
               else MoveMemory (&(BitmapArray[(i*Width+w)*3]), &(BitmapArray[w*3]), -a * 3);
            }
         }

         //SetThreadPriority (MyThread, THREAD_PRIORITY_IDLE);
         
         Rect.left = 0;
         Rect.top = 0;
         Rect.right = Width;
         Rect.bottom = Height;
      }
   }

   void TurtleAt (double x, double y)
   {
      TurtleX = x;
      TurtleY = y;
   }

   void CalibrateAngle (double &angle)
   {
      angle -= 2 * Pi * floor (angle / (2 * Pi));
   }

   void TurtleTurn (double angle)
   {
      TurtleDirection += angle;
      CalibrateAngle (TurtleDirection);
   }

   void TurtleTurnTo (double angle)
   {
      TurtleDirection = angle;
      CalibrateAngle (TurtleDirection);
   }

   void TurtleDown ()
   {
      TurtlePenTracing = TRUE;
   }

   void TurtleUp ()
   {
      TurtlePenTracing = FALSE;
   }

   void TurtleMove (double d, BYTE r = 255, BYTE g = 255, BYTE b = 255)
   {
      double TurtleXOld = TurtleX;
      double TurtleYOld = TurtleY;

      TurtleX += d * cos (TurtleDirection);
      TurtleY += d * sin (TurtleDirection);

      if (TurtlePenTracing) Line ((int) TurtleXOld, (int) TurtleYOld, (int) TurtleX, (int) TurtleY, r, g, b);
   }

};

Bitmap *B;

int kc;
double Pi;

HANDLE MyThread;
volatile BOOL ThreadSuspended = FALSE;
volatile BOOL ThreadSaysProgramMustStop = FALSE;
volatile BOOL ThreadFinishedItsJob = FALSE;
volatile BOOL ThreadShouldLoopWhilePainting = FALSE;
volatile BOOL ThreadWantsRedrawNow = FALSE;
HWND    hWnd;

malabar RefreshInterval =  100;
malabar RefreshNextTime =    0;
BOOL    RefreshByUser =  FALSE;
BOOL    RefreshWaiting = FALSE;

int MouseX = 0;
int MouseY = 0;

BOOL MouseL = FALSE;
BOOL MouseM = FALSE;
BOOL MouseR = FALSE;
BOOL MouseLp = FALSE;
BOOL MouseMp = FALSE;
BOOL MouseRp = FALSE;

char KeyboardBuffer [1024];
int OldestKey = 0;
int NextKey = 0;

RECT r;

inline BOOL ThereAreKeys ()
{
   return ((OldestKey != NextKey) ? TRUE : FALSE);
}

void AddKey(char c)
{
   KeyboardBuffer [NextKey++] = c;
   if (NextKey > 1023) NextKey = 0;
}

char TakeKey ()
{
   char c;
   if (ThereAreKeys())
   {
      c = KeyboardBuffer [OldestKey++];
      if (OldestKey > 1023) OldestKey = 0;
   }
   else
   {
      c = 0;
   }
   return c;
}

template <class cx, class cy, class cl, class cm, class cr>
void Mouse (cx &x, cy &y, cl &l, cm &m, cr &r)
{
   x = (cx) MouseX;
   y = (cy) MouseY;
   l = (cl) MouseL;
   m = (cm) MouseM;
   r = (cr) MouseR;
}

template <class cx, class cy>
void Mouse (cx &x, cy &y)
{
   x = (cx) MouseX;
   y = (cy) MouseY;
}

char Inkey ()
{
   char r;
   r = 0;
   if (ThereAreKeys ()) r = TakeKey();
   return r;
}

template <class cr, class cg, class cb>
inline void Calibrate (cr &r, cg &g, cb &b)
{
   if (r > (cr) 255) r = (cr) 255;
   if (g > (cr) 255) g = (cr) 255;
   if (b > (cr) 255) b = (cr) 255;
   if (r < (cr) 0) r = (cr) 0;
   if (g < (cr) 0) g = (cr) 0;
   if (b < (cr) 0) b = (cr) 0;
}

template <class cr, class cg, class cb>
void Ink (cr r, cg g, cb b)
{
   Calibrate (r, g, b);
   B->Ink ((BYTE) r, (BYTE) g, (BYTE) b);
}

int InkR ()
{
   return B->InkR;
}

int InkG ()
{
   return B->InkG;
}

int InkB ()
{
   return B->InkB;
}

int PaperR ()
{
   return B->PaperR;
}

int PaperG ()
{
   return B->PaperG;
}

int PaperB ()
{
   return B->PaperB;
}

template <class cr, class cg, class cb>
void Paper (cr r, cg g, cb b)
{
   Calibrate (r, g, b);
   B->Paper ((BYTE) r, (BYTE) g, (BYTE) b);
}

template <class cx, class cy, class cr, class cg, class cb>
inline void Plot (cx x, cy y, cr r, cg g, cb b)
{
   Calibrate (r, g, b);
   B->Plot ((int) x, (int) y, (BYTE) r, (BYTE) g, (BYTE) b);
}

template <class cx, class cy>
inline void Plot (cx x, cy y)
{
   B->Plot ((int) x, (int) y, B->InkR, B->InkG, B->InkB);
}

template <class cx, class cy, class cr, class cg, class cb>
void Pixel (cx x, cy y, cr &r, cg &g, cb &b)
{
   BYTE lr, lg, lb;
   B->Pixel ((int) x, (int) y, lr, lg, lb);
   r = (cr) lr;
   g = (cg) lg;
   b = (cb) lb;
}

template <class cx, class cy>
int PixelR (cx x, cy y)
{
   return B->PixelR ((int) x, (int) y);
}

template <class cx, class cy>
int PixelG (cx x, cy y)
{
   return B->PixelG ((int) x, (int) y);
}

template <class cx, class cy>
int PixelB (cx x, cy y)
{
   return B->PixelB ((int) x, (int) y);
}

template <class cx, class cy, class cd>
void PlotPrint (cx x, cy y, cd n)
{
   char a[128];
   ostrstream b(a, 128);

   B->seekp(0);
   b << n << ends;

   B->Print ((int)x, (int)y, (char *)a);
}

template <class cx, class cy, class cd, class cfn, class cs, class cb, class ci, class cu>
void FontPrint (cx x, cy y, cd n, cfn fn, cs s, cb b, ci i, cu u)
{
   char a[16385];
   ostrstream bb(a, 16384);

   char fa[257];
   ostrstream fb(fa, 256);

   bB->seekp(0);
   bb << n << ends;

   fB->seekp (0);
   fb << fn << ends;

   B->FontPrint((int) x, (int) y, (char *) a, (char *) fa, (int) s, (BOOL) b, (BOOL) i,(BOOL) u);
}

template <class cd>
void _Print (cd n)
{
   char a[128];
   ostrstream b(a, 128);

   B->seekp(0);
   b << n << ends;

   B->Print ((char *)a);
}

template <class cl, class cc>
void Locate (cl l, cc c)
{
   B->Locate ((int) l, (int) c);
}

int LocateL ()
{
   return B->LocateL;
}

int LocateC ()
{
   return B->LocateC;
}

template <class cv>
void InputNumber (cv &v)
{
   char InputBuffer[1024];
   istrstream InputStream (InputBuffer, 1023);
   B->Input (InputBuffer, 1023); 
   InputStream.seekg(0); 
   InputStream >> v;
}

void InputString (char* v, int l)
{
   B->Input (v, l);
}

class ConsoleStream {};
ConsoleStream Print;
template <class ca>
ConsoleStream &operator << (ConsoleStream &c, ca a)
{
   _Print (a);
   return c;
}

template <class cw>
void SetLineWidth(cw w)
{
   B->LineWidth = (int) w;
}

int LineWidth()
{
   return B->LineWidth;
}

template <class cx1, class cy1, class cx2, class cy2, class cr, class cg, class cb>
void Line (cx1 x1, cy1 y1, cx2 x2, cy2 y2, cr r, cg g, cb b)
{
   Calibrate (r, g, b);
   B->Line ((int) x1, (int) y1, (int) x2, (int) y2, (BYTE) r, (BYTE) g, (BYTE) b);
}

template <class cx1, class cy1, class cx2, class cy2>
void Line (cx1 x1, cy1 y1, cx2 x2, cy2 y2)
{
   B->Line ((int) x1, (int) y1, (int) x2, (int) y2, B->InkR, B->InkG, B->InkB);
}

template <class cxc, class cyc, class cra, class cr, class cg, class cb>
void Circle (cxc xc, cyc yc, cra ra, cr r, cg g, cb b)
{
   Calibrate (r, g, b);
   B->Circle ((int) xc, (int) yc, (int) ra, (BYTE) r, (BYTE) g, (BYTE) b);
}

template <class cxc, class cyc, class crc>
void Circle (cxc xc, cyc yc, crc rc)
{
   B->Circle ((int) xc, (int) yc, (int) rc, B->InkR, B->InkG, B->InkB);
}

template <class cr, class cg, class cb>
void Cls (cr r, cg g, cb b)
{
   Calibrate (r, g, b);
   B->Cls((BYTE) r, (BYTE)g, (BYTE)b);
}

void Cls ()
{
   B->Cls(B->PaperR, B->PaperG, B->PaperB);
}

template <class ca, class cr, class cg, class cb>
void Scroll (ca a, cr r, cg g, cb b)
{
   Calibrate (r, g, b);
   B->Scroll((int) a, (BYTE) r, (BYTE)g, (BYTE)b);
}

template <class ca>
void Scroll (ca a)
{
   B->Scroll((int) a, B->PaperR, B->PaperG, B->PaperB);
}

template <class ca, class cr, class cg, class cb>
void Pann (ca a, cr r, cg g, cb b)
{
   Calibrate (r, g, b);
   B->Pann((int) a, (BYTE) r, (BYTE)g, (BYTE)b);
}

template <class ca>
void Pann (ca a)
{
   B->Pann((int) a, B->PaperR, B->PaperG, B->PaperB);
}

template <class cx, class cy>
void TurtleAt (cx x, cy y)
{
   B->TurtleAt ((double) x, (double) y);
}

template <class ca>
void TurtleTurn (ca a)
{
   B->TurtleTurn ((double) a);
}

template <class ca>
void TurtleTurnTo (ca a)
{
   B->TurtleTurnTo ((double) a);
}

void TurtleDown ()
{
   B->TurtleDown ();
}

void TurtleUp () 
{
   B->TurtleUp ();
}

BOOL TurtleIsDown ()
{
   return B->TurtlePenTracing;
}

BOOL TurtleIsUp ()
{
   return !B->TurtlePenTracing;
}

double TurtleX()
{
   return B->TurtleX;
}

double TurtleY()
{
   return B->TurtleY;
}

double TurtleDirection ()
{
   return B->TurtleDirection;
}

template <class ca, class cr, class cg, class cb>
void TurtleMove (ca a, cr r, cg g, cb b)
{
   Calibrate (r, g, b);
   B->TurtleMove((double) a, (BYTE) r, (BYTE)g, (BYTE)b);
}

template <class ca>
void TurtleMove (ca a)
{
   B->TurtleMove ((double) a, B->PaperR, B->PaperG, B->PaperB);
}

void RedrawNow ()
{
   if (B->Rect.top <= B->Rect.bottom) 
   {
      InvalidateRect(hWnd, &(B->Rect), FALSE);
   
      B->Rect.top = 200;
      B->Rect.left = 320;
      B->Rect.bottom = 0;
      B->Rect.right = 0;
   }
}

void Refresh()
{
   malabar Time = MilliTime ();
   ThreadWantsRedrawNow = TRUE;
   RefreshNextTime = Time + RefreshInterval;
   ThreadShouldLoopWhilePainting = TRUE;
   PostMessage(hWnd,    WM_TIMER, 0,0);
   while (ThreadShouldLoopWhilePainting);
}

void Quit()
{
   ThreadSaysProgramMustStop = TRUE;
   PostMessage(hWnd,    WM_TIMER, 0,0);
   while(TRUE);
}

template <class cv1, class cv2>
void Val (cv1 &v1, cv2 v2)
{
   char a[16385];

   ostrstream bo(a, 16383);
   bo.seekp(0);
   bo << v2 << ends;

   istrstream bi(a);
   bi.seekg(0);
   bi >> v1;
}

void SuspendMyThread()
{
   if (!ThreadSuspended)
   {
      SuspendThread(MyThread);
      ThreadSuspended = TRUE;
   }
}

void ResumeMyThread()
{
   if (ThreadSuspended)
   {
      SuspendThread (MyThread);
      ThreadSuspended = FALSE;
   }
}

void ManageThreadAndRefresh()
{
   malabar Time = MilliTime ();
   static DidLastRefresh = FALSE;

   if (ThreadWantsRedrawNow)
   {
      RedrawNow();
      ThreadWantsRedrawNow = FALSE;
      RefreshWaiting = FALSE;
      RefreshNextTime = Time + RefreshInterval;
   }

   if (RefreshByUser)
   {
      if (Time >= RefreshNextTime)
      {
         RefreshWaiting = TRUE;
      }
   }

   else

   {
      if (Time >= RefreshNextTime && !ThreadFinishedItsJob)
      {
         RedrawNow();
         RefreshWaiting = FALSE;
         RefreshNextTime = Time + RefreshInterval;
      }
   }

   if (ThreadFinishedItsJob)
   {
      if (!RefreshByUser && !DidLastRefresh)
      {
         RedrawNow ();
         DidLastRefresh = TRUE;
      }
      SuspendMyThread ();
   }

   if (ThreadSaysProgramMustStop)
   {
      SuspendMyThread ();
      PostQuitMessage (0);
   }
}

VOID CALLBACK TimerProc (HWND  hwnd, UINT  uMsg, UINT  idEvent, DWORD  dwTime)
{
   ManageThreadAndRefresh();
}

void MainThread ();

DWORD Thread (LPVOID data)
{
   // GWF - added loop to init to black
   for(int i = 0; i < win98height; i++)
     Line(0, i, win98width - 1, i, 0, 0, 0);

   MainThread ();

   ThreadFinishedItsJob = TRUE;   

   PostMessage(hWnd,    WM_TIMER, 0,0);
   
   while (TRUE);

   ExitThread (0);
   return 0;
}

// Prototype de la fonction de fenêtre
LONG FAR PASCAL WndProc(HWND, UINT, WPARAM, LPARAM);

static LPSTR cmdline;  // GWF - added

// Fonction principale du programme 
int PASCAL WinMain(HANDLE hInstance, HANDLE hPrevInstance, 
                   LPSTR lpszCmdLine, int nCmdShow)
{
   char *p;

   if((p = getenv("CBNWIDTH")) != NULL)
     win98width = atoi(p);
   if((p = getenv("CBNHEIGHT")) != NULL)
     win98height = atoi(p);
   B = new Bitmap(win98width, win98height);

   Pi = atan (1) * 4;

   cmdline = lpszCmdLine; // GWF - added

   // Déclaration des variables locales à WinMain
   static char szAppName[] = "Plot";
   // HWND      hWnd;
   MSG  msg;
   WNDCLASS     wndclass;

   UINT TimerNumber;

   B->Cls();

   if(!hPrevInstance)
   {
      // Enregistrement de la classe de fenêtre

      // Styles de la classe de fenêtre 
      wndclass.style = CS_BYTEALIGNCLIENT       | CS_HREDRAW | CS_VREDRAW;

      // Nom de la procédure de fenêtre 
      wndclass.lpfnWndProc = WndProc; 
      wndclass.cbClsExtra = 0;
      wndclass.cbWndExtra = 0;
      wndclass.hInstance = hInstance;

      // Icône par défaut du programme 
      wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);

      // Curseur par défaut du programme 
      wndclass.hCursor = LoadCursor (NULL, IDC_ARROW);

      // Brosse pour la couleur du fond
      wndclass.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1); // GetStockObject (WHITE_BRUSH);
      
      // Pas de menu
      wndclass.lpszMenuName = NULL;

      // Nom de la classe de fenêtre
      wndclass.lpszClassName = szAppName;

      // Enregistrement de la classe de fenêtre
      RegisterClass (&wndclass);
   }

   // Calculate size
   r.top = 0; 
   r.left = 0;
   r.bottom = B->Height;
   r.right = B->Width;
   AdjustWindowRect (&r, WS_OVERLAPPEDWINDOW, 0);

   // Création de la fenêtre
   hWnd = CreateWindow (szAppName,
          "Plot",
          WS_OVERLAPPEDWINDOW,
          CW_USEDEFAULT, CW_USEDEFAULT, 
          r.right - r.left, r.bottom - r.top, 
          NULL, 
          NULL, 
          hInstance, 
          NULL);
         
   // Affichage de la fenêtre
   ShowWindow(hWnd, nCmdShow);

   // Mise à jour du contenu de la fenêtre
   UpdateWindow (hWnd);

   // launch trhead
   DWORD identifier;
   MyThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) Thread, NULL, 0, &identifier);
   SetThreadPriority (MyThread, THREAD_PRIORITY_IDLE); //BELOW_NORMAL);
   ThreadSuspended = FALSE;
   RefreshNextTime = MilliTime () + RefreshInterval;

   // Set Timer, 1 second
   TimerNumber = SetTimer (NULL, 0, 10, (TIMERPROC) TimerProc);
   //tTimer (NULL, 0, 1, NULL);

   KeyboardBuffer[0] = 0;

   // Boucle des messages
   while(GetMessage(&msg, NULL, 0, 0))
   {         
      TranslateMessage (&msg);

      switch (msg.message)
      {   
      case WM_TIMER:

      default:

         DispatchMessage (&msg);
      }

      ManageThreadAndRefresh();

      if (ThreadFinishedItsJob) if (Inkey() == 27) PostQuitMessage(0);
   }

   SuspendThread (MyThread);
   CloseHandle (MyThread);
   KillTimer (NULL, TimerNumber);
   return(msg.wParam);
}
       
// Développement de la fonction de fenêtre
LONG FAR PASCAL WndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{         
   // Handle pour le contexte d'affichage
   HDC            hDC;
   PAINTSTRUCT    ps;
   RECT           taille_fenetre;

   static long passages = 0;
   // char s[1024];
   char c;

   switch (Message)
   {    
   case WM_PAINT:

      passages++;

      hDC = BeginPaint (hWnd, &ps);
      GetClientRect (hWnd, &taille_fenetre);    

      StretchDIBits (hDC, 
                     0, 0, 
                     B->Width, B->Height,
                     0, 0,
                     B->Width, B->Height,
                     B->BitmapArray,
                     &(B->BitmapInfo),
                     DIB_RGB_COLORS,
                     SRCCOPY);
      EndPaint (hWnd, &ps);

      if (ThreadShouldLoopWhilePainting) 
      {
         ThreadShouldLoopWhilePainting = FALSE;
         ResumeMyThread ();
      }

      break;
         
   case WM_CHAR:

      c = (TCHAR) wParam;  // TCHAR

      if (c != 0 && strlen (KeyboardBuffer) < 1000)
      {
         if (!(ThereAreKeys() && ((lParam >> 30) & 1))) AddKey (c);
      }

      return 0;
      break;

   case WM_MOUSEMOVE:

      if ((wParam & MK_LBUTTON) == 0) MouseL = FALSE;        // key flags 
      else                                {MouseL = TRUE; MouseLp = TRUE;}

      if ((wParam & MK_MBUTTON) == 0) MouseM = FALSE;        // key flags 
      else                                {MouseM = TRUE; MouseMp = TRUE;}

      if ((wParam & MK_RBUTTON) == 0) MouseR = FALSE;        // key flags 
      else                                {MouseR = TRUE; MouseRp = TRUE;}

      MouseX = LOWORD(lParam);  // horizontal position of cursor 
      MouseY = win98height - HIWORD(lParam) - 1;  // vertical position of cursor 

      return 0;
      break;

   case WM_LBUTTONDOWN:
      MouseL = TRUE;
      MouseLp = TRUE;
      return 0;
      break;

   case WM_LBUTTONUP:
      MouseL = FALSE;
      return 0;
      break;

   case WM_MBUTTONDOWN:
      MouseM = TRUE;
      MouseMp = TRUE;
      return 0;
      break;

   case WM_MBUTTONUP:
      MouseM = FALSE;
      return 0;
      break;

   case WM_RBUTTONDOWN:
      MouseR = TRUE;
      MouseRp = TRUE;
      return 0;
      break;

   case WM_RBUTTONUP:
      MouseR = FALSE;
      return 0;
      break;
      
   case WM_DESTROY:
      SuspendThread (MyThread);
      PostQuitMessage (0);
      break;

   default:
      return DefWindowProc(hWnd, Message, wParam, lParam);
   }

   return NULL;
}         
        
