#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include <SOIL.h>

typedef struct {
  unsigned char r, g, b;
} RGB8;

typedef struct {
  int width, height;
  RGB8* img;
} Img;

typedef struct {
  unsigned char r, g, b;
} RGB;

int energyCalculator(RGB previousPixel,
                     RGB nextPixel,
                     RGB topPixel,
                     RGB lowerPixel);
int sumEnergy(int* energy, int maxWidth, int actualWidth, int i);
void load(char* name, Img* pic);
void uploadTexture();
void seamcarve(int targetWidth);
void freemem();

// Funções da interface gráfica e OpenGL
void init();
void draw();
void keyboard(unsigned char key, int x, int y);
void arrow_keys(int a_keys, int x, int y);

int width, height;
int targetW = 400;

GLuint tex[3];
Img pic[3];
Img* source;
Img* mask;
Img* target;

// Imagem selecionada (0,1,2)
int sel;

// Carrega uma imagem para a struct Img
void load(char* name, Img* pic) {
  int chan;
  pic->img = (RGB8*)SOIL_load_image(name, &pic->width, &pic->height, &chan,
                                    SOIL_LOAD_RGB);
  if (!pic->img) {
    printf("SOIL loading error: '%s'\n", SOIL_last_result());
    exit(1);
  }
  printf("Load: %d x %d x %d\n", pic->width, pic->height, chan);
}

void freemem() {
  // Libera a memória ocupada pelas 3 imagens
  free(pic[0].img);
  free(pic[1].img);
  free(pic[2].img);
}

/********************************************************************
 *
 *  VOCÊ NÃO DEVE ALTERAR NADA NO PROGRAMA A PARTIR DESTE PONTO!
 *
 ********************************************************************/
int main(int argc, char** argv) {
  if (argc < 2) {
    printf("seamcarving [origem] [mascara]\n");
    printf("Origem é a imagem original, mascara é a máscara desejada\n");
    exit(1);
  }
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

  load(argv[1], &pic[0]);
  load(argv[2], &pic[1]);

  if (pic[0].width != pic[1].width || pic[0].height != pic[1].height) {
    printf("Imagem e máscara com dimensões diferentes!\n");
    exit(1);
  }

  width = pic[0].width;
  height = pic[0].height;
  pic[2].width = pic[1].width;
  pic[2].height = pic[1].height;
  source = &pic[0];
  mask = &pic[1];
  target = &pic[2];
  targetW = target->width;

  glutInitWindowSize(width, height);
  glutCreateWindow("Seam Carving");
  glutDisplayFunc(draw);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(arrow_keys);

  tex[0] = SOIL_create_OGL_texture((unsigned char*)pic[0].img, pic[0].width,
                                   pic[0].height, SOIL_LOAD_RGB,
                                   SOIL_CREATE_NEW_ID, 0);
  tex[1] = SOIL_create_OGL_texture((unsigned char*)pic[1].img, pic[1].width,
                                   pic[1].height, SOIL_LOAD_RGB,
                                   SOIL_CREATE_NEW_ID, 0);

  printf("Origem  : %s %d x %d\n", argv[1], pic[0].width, pic[0].height);
  printf("Máscara : %s %d x %d\n", argv[2], pic[1].width, pic[0].height);
  sel = 0;

  glMatrixMode(GL_PROJECTION);
  gluOrtho2D(0.0, width, height, 0.0);
  glMatrixMode(GL_MODELVIEW);

  pic[2].img =
      malloc(pic[1].width * pic[1].height * 3);  // W x H x 3 bytes (RGB)
  memset(pic[2].img, 0, width * height * 3);
  tex[2] = SOIL_create_OGL_texture((unsigned char*)pic[2].img, pic[2].width,
                                   pic[2].height, SOIL_LOAD_RGB,
                                   SOIL_CREATE_NEW_ID, 0);

  glutMainLoop();
}

void keyboard(unsigned char key, int x, int y) {
  if (key == 27) {
    // ESC: libera memória e finaliza
    freemem();
    exit(1);
  }
  if (key >= '1' && key <= '3')
    sel = key - '1';

  if (key == 's') {
    seamcarve(targetW);
  }
  glutPostRedisplay();
}

void seamcarve(int targetWidth) {
  // RGB8(*ptr)[target->width] = (RGB8(*)[target->width])target->img;

  // for (int y = 0; y < target->height; y++) {
  //     for (int x = 0; x < targetW; x++) ptr[y][x].r = ptr[y][x].g = 255;
  //     for (int x = targetW; x < target->width; x++)
  //         ptr[y][x].r = ptr[y][x].g = 0;
  // }
  int auxWidth = pic[2].width;
  int desiredwidth = targetWidth, lowestEnergyPixel;
  RGB* mask = malloc(pic[1].width * pic[1].height * 3);

  for (int i = 0; i < pic[2].height * pic[2].width; i++) {
  }

  uploadTexture();
  glutPostRedisplay();
}

int energyCalculator(RGB previousPixel,
                     RGB nextPixel,
                     RGB topPixel,
                     RGB lowerPixel) {
  return (pow(previousPixel.r - nextPixel.r, 2)) +
         (pow(previousPixel.g - nextPixel.g, 2)) +
         (pow(previousPixel.b - nextPixel.b, 2)) +
         (pow(previousPixel.r - nextPixel.r, 2)) +
         (pow(previousPixel.g - nextPixel.g, 2)) +
         (pow(previousPixel.b - nextPixel.b, 2)) +
         (pow(topPixel.r - lowerPixel.r, 2)) +
         (pow(topPixel.g - lowerPixel.g, 2)) +
         (pow(topPixel.b - lowerPixel.b, 2));
}

int sumEnergy(int* energy, int maxWidth, int actualWidth, int i) {
  int finalEnergy = energy[i - maxWidth];
  if (i % maxWidth != 0 && energy[i - maxWidth - 1] < finalEnergy) {
    finalEnergy = energy[i - maxWidth - 1];
  }

  if (i % maxWidth != (actualWidth - 1) &&
      energy[i - maxWidth * 1] < finalEnergy) {
    finalEnergy = energy[i - maxWidth * 1];
  }

  return finalEnergy;
}

void arrow_keys(int a_keys, int x, int y) {
  switch (a_keys) {
    case GLUT_KEY_RIGHT:
      if (targetW <= pic[2].width - 10)
        targetW += 10;
      seamcarve(targetW);
      break;
    case GLUT_KEY_LEFT:
      if (targetW > 10)
        targetW -= 10;
      seamcarve(targetW);
      break;
    default:
      break;
  }
}

void uploadTexture() {
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, tex[2]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, target->width, target->height, 0,
               GL_RGB, GL_UNSIGNED_BYTE, target->img);
  glDisable(GL_TEXTURE_2D);
}

void draw() {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glColor3ub(255, 255, 255);
  glBindTexture(GL_TEXTURE_2D, tex[sel]);
  glEnable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0);
  glVertex2f(0, 0);
  glTexCoord2f(1, 0);
  glVertex2f(pic[sel].width, 0);
  glTexCoord2f(1, 1);
  glVertex2f(pic[sel].width, pic[sel].height);
  glTexCoord2f(0, 1);
  glVertex2f(0, pic[sel].height);
  glEnd();
  glDisable(GL_TEXTURE_2D);
  glutSwapBuffers();
}
