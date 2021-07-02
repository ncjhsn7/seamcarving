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
#include <math.h>

typedef struct {
  unsigned char r, g, b;
} RGB8;

typedef struct {
  int width, height;
  RGB8* img;
} Img;

int energyCalculator(RGB8 previousPixel,
                     RGB8 nextPixel,
                     RGB8 topPixel,
                     RGB8 lowerPixel);
int accumulatedEnergy(int* energy, int maxWidth, int actualWidth, int i);
void load(char* name, Img* pic);
void uploadTexture();
void seamcarve();
void freemem();
void removeLine(int direction);

// Funções da interface gráfica e OpenGL
void init();
void draw();
void keyboard(unsigned char key, int x, int y);
void arrow_keys(int a_keys, int x, int y);

int width, height;
int targetW = 400;
int dw = 10;
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
  // int auxWidth = pic[2].width;
  if (key == 27) {
    // ESC: libera memória e finaliza
    freemem();
    exit(1);
  }
  if (key >= '1' && key <= '3')
    sel = key - '1';

  if (key == 's') {
    seamcarve();
  }
  glutPostRedisplay();
}

void arrow_keys(int a_keys, int x, int y) {
  switch (a_keys) {
    case GLUT_KEY_RIGHT:
      // if (targetW <= pic[2].width - 10)
      dw = 10;
      removeLine(dw);
      dw = 0;
      break;
    case GLUT_KEY_LEFT:
      // if (targetW > 10)
      // targetW -= 10;
      dw = -10;
      removeLine(dw);
      dw = 0;
      break;
    default:
      break;
  }
}

void removeLine(int direction) {
  int auxWidth = pic[2].width;
  int lowestEnergyPixel = 0;
  printf("TARGET W VALUE: %d \n", dw);
  printf("PICTURE WIDTH: %d \n", pic[2].width);
  // pic[2].width += direction;
  // auxWidth += direction;
  RGB8* maskAux = malloc(pic[1].width * pic[1].height * 3);

  for (int i = 0; i < pic[2].height * pic[2].width; i++)
    pic[2].img[i] = pic[0].img[i];

  int* energy = malloc(sizeof(int) * pic[2].height * pic[2].width);

  for (int i = 0; i < pic[2].height; i++) {
    for (int j = 0; j < pic[2].width; j++) {
      if (pic[1].img[(i * pic[2].width) + j].r > 200 &&
          pic[1].img[(i * pic[2].width) + j].g < 100 &&
          pic[1].img[(i * pic[2].width) + j].b < 100)
        energy[(i * pic[2].width) + j] = -10000000;

      else if (pic[1].img[(i * pic[2].width) + j].r < 100 &&
               pic[1].img[(i * pic[2].width) + j].g > 200 &&
               pic[1].img[(i * pic[2].width) + j].b < 100)
        energy[(i * pic[2].width) + j] = 10000000;

      else
        energy[(i * pic[2].width) + j] = 0;

      if (i == 0) {
        if (j == 0)
          energy[0] += energyCalculator(pic[2].img[1], pic[2].img[auxWidth - 1], pic[2].img[pic[2].width * (pic[2].height - 1)],
                               pic[2].img[pic[2].width]);

        else if (j == auxWidth - 1)
          energy[j] += energyCalculator(
              pic[2].img[auxWidth - 2], pic[2].img[0],
              pic[2].img[(pic[2].width * pic[2].height - 1) + auxWidth - 1],
              pic[2].img[(pic[2].width + auxWidth) - 1]);

        else
          energy[j] += energyCalculator(
              pic[2].img[j - 1], pic[2].img[j + 1],
              pic[2].img[j + (pic[2].width * (pic[2].height - 1))],
              pic[2].img[j + pic[2].width]);

      } else if ((i * pic[2].width) + j >= pic[2].width * (pic[2].height - 1)) {
        if (j == 0) {
          energy[i * pic[2].width] += energyCalculator(
              pic[2].img[(pic[2].width * i) + auxWidth - 1],
              pic[2].img[(i * pic[2].width) + j + 1],
              pic[2].img[pic[2].width * (i - 1)], pic[2].img[0]);
          energy[i * pic[2].width] += accumulatedEnergy(
              energy, pic[2].width, auxWidth, (i * pic[2].width) + j);

        }

        else if (j == auxWidth - 1) {
          energy[(i * pic[2].width) + j] += energyCalculator(
              pic[2].img[(pic[2].width * i) + auxWidth - 2],
              pic[2].img[pic[2].width * i],
              pic[2].img[(pic[2].width * (i - 1)) + auxWidth - 1],
              pic[2].img[j]);
          energy[(i * pic[2].width) + j] += accumulatedEnergy(
              energy, pic[2].width, auxWidth, (i * pic[2].width) + j);

        } else {
          energy[(i * pic[2].width) + j] += energyCalculator(
              pic[2].img[(i * pic[2].width) + j - 1],
              pic[2].img[(i * pic[2].width) + j + 1],
              pic[2].img[((i - 1) * pic[2].width) + j], pic[2].img[j]);
          energy[(i * pic[2].width) + j] += accumulatedEnergy(
              energy, pic[2].width, auxWidth, (i * pic[2].width) + j);
        }

      }

      else if (j == 0) {
        energy[i * pic[2].width] +=
            energyCalculator(pic[2].img[(i * pic[2].width) + auxWidth - 1],
                             pic[2].img[(i * pic[2].width) + 1],
                             pic[2].img[(i - 1) * pic[2].width],
                             pic[2].img[(i + 1) * pic[2].width]);
        energy[i * pic[2].width] += accumulatedEnergy(
            energy, pic[2].width, auxWidth, (i * pic[2].width) + j);
        ;

      }

      else if (j == auxWidth - 1) {
        energy[(i * pic[2].width) + j] +=
            energyCalculator(pic[2].img[(i * pic[2].width) + j - 1],
                             pic[2].img[i * pic[2].width],
                             pic[2].img[(i - 1) * pic[2].width + j],
                             pic[2].img[(i + 1) * pic[2].width + j]);
        energy[(i * pic[2].width) + j] += accumulatedEnergy(
            energy, pic[2].width, auxWidth, (i * pic[2].width) + j);
        ;

      }

      else {
        energy[(i * pic[2].width) + j] +=
            energyCalculator(pic[2].img[(i * pic[2].width) + j - 1],
                             pic[2].img[(i * pic[2].width) + j + 1],
                             pic[2].img[(i - 1) * pic[2].width + j],
                             pic[2].img[(i + 1) * pic[2].width + j]);
        energy[(i * pic[2].width) + j] += accumulatedEnergy(
            energy, pic[2].width, auxWidth, (i * pic[2].width) + j);
        ;
      }
    }
  }

  lowestEnergyPixel = pic[2].width * (pic[2].height - 1);

  for (int i = lowestEnergyPixel;
       i < pic[2].width * (pic[2].height - 1) + auxWidth; i++) {
    if (energy[i] < energy[lowestEnergyPixel])
      lowestEnergyPixel = i;
  }

  for (int i = lowestEnergyPixel;;) {
    int column = i % pic[2].width;

    for (int j = 0; j < auxWidth - column; j++) {
      pic[2].img[i + j] = pic[2].img[i + j + 1];
      pic[1].img[i + j] = pic[1].img[i + j + 1];
    }

    pic[2].img[(i - column) + auxWidth - 1].r = 0;
    pic[2].img[(i - column) + auxWidth - 1].g = 0;
    pic[2].img[(i - column) + auxWidth - 1].b = 0;

    if (i - pic[2].width < 0)
      break;

    int lowestPath = i - pic[2].width;
    int lowestEnergy = energy[i - pic[2].width];

    if (i % pic[2].width != 0 && energy[i - pic[2].width - 1] < lowestEnergy) {
      lowestEnergy = energy[i - pic[2].width - 1];
      lowestPath = i - pic[2].width - 1;
    }

    if (i % pic[2].width != (auxWidth - 1) &&
        energy[i - pic[2].width + 1] < lowestEnergy) {
      lowestEnergy = energy[i - pic[2].width + 1];
      lowestPath = i - pic[2].width + 1;
    }

    i = lowestPath;
  }
  free(energy);
  uploadTexture();
  
  free(maskAux);
}

void seamcarve() {
  int auxWidth = pic[2].width;
  int desiredWidth = 400, lowestEnergyPixel;
  RGB8* maskAux = malloc(pic[1].width * pic[1].height * 3);

  for (int i = 0; i < pic[2].height * pic[2].width; i++)
    pic[2].img[i] = pic[0].img[i];

  while (auxWidth > desiredWidth) {
    printf("(while) -> auxWidth: %d\n", auxWidth);
    printf("(while) -> desiredWidth: %d\n", desiredWidth);
    int* energy = malloc(sizeof(int) * pic[2].height * pic[2].width);

    for (int i = 0; i < pic[2].height; i++) {
      for (int j = 0; j < pic[2].width; j++) {
        if (pic[1].img[(i * pic[2].width) + j].r > 200 &&
            pic[1].img[(i * pic[2].width) + j].g < 100 &&
            pic[1].img[(i * pic[2].width) + j].b < 100)
          energy[(i * pic[2].width) + j] = -1000000;

        else if (pic[1].img[(i * pic[2].width) + j].r < 100 &&
                 pic[1].img[(i * pic[2].width) + j].g > 200 &&
                 pic[1].img[(i * pic[2].width) + j].b < 100)
          energy[(i * pic[2].width) + j] = 10000000;

        else
          energy[(i * pic[2].width) + j] = 0;

        if (i == 0) {
          if (j == 0)
            energy[0] +=
                energyCalculator(pic[2].img[1], pic[2].img[auxWidth - 1],
                                 pic[2].img[pic[2].width * (pic[2].height - 1)],
                                 pic[2].img[pic[2].width]);

          else if (j == auxWidth - 1)
            energy[j] += energyCalculator(
                pic[2].img[auxWidth - 2], pic[2].img[0],
                pic[2].img[(pic[2].width * pic[2].height - 1) + auxWidth - 1],
                pic[2].img[(pic[2].width + auxWidth) - 1]);

          else
            energy[j] += energyCalculator(
                pic[2].img[j - 1], pic[2].img[j + 1],
                pic[2].img[j + (pic[2].width * (pic[2].height - 1))],
                pic[2].img[j + pic[2].width]);

        } else if ((i * pic[2].width) + j >=
                   pic[2].width * (pic[2].height - 1)) {
          if (j == 0) {
            energy[i * pic[2].width] += energyCalculator(
                pic[2].img[(pic[2].width * i) + auxWidth - 1],
                pic[2].img[(i * pic[2].width) + j + 1],
                pic[2].img[pic[2].width * (i - 1)], pic[2].img[0]);
            energy[i * pic[2].width] += accumulatedEnergy(
                energy, pic[2].width, auxWidth, (i * pic[2].width) + j);

          }

          else if (j == auxWidth - 1) {
            energy[(i * pic[2].width) + j] += energyCalculator(
                pic[2].img[(pic[2].width * i) + auxWidth - 2],
                pic[2].img[pic[2].width * i],
                pic[2].img[(pic[2].width * (i - 1)) + auxWidth - 1],
                pic[2].img[j]);
            energy[(i * pic[2].width) + j] += accumulatedEnergy(
                energy, pic[2].width, auxWidth, (i * pic[2].width) + j);

          } else {
            energy[(i * pic[2].width) + j] += energyCalculator(
                pic[2].img[(i * pic[2].width) + j - 1],
                pic[2].img[(i * pic[2].width) + j + 1],
                pic[2].img[((i - 1) * pic[2].width) + j], pic[2].img[j]);
            energy[(i * pic[2].width) + j] += accumulatedEnergy(
                energy, pic[2].width, auxWidth, (i * pic[2].width) + j);
          }

        }

        else if (j == 0) {
          energy[i * pic[2].width] +=
              energyCalculator(pic[2].img[(i * pic[2].width) + auxWidth - 1],
                               pic[2].img[(i * pic[2].width) + 1],
                               pic[2].img[(i - 1) * pic[2].width],
                               pic[2].img[(i + 1) * pic[2].width]);
          energy[i * pic[2].width] += accumulatedEnergy(
              energy, pic[2].width, auxWidth, (i * pic[2].width) + j);
          ;

        }

        else if (j == auxWidth - 1) {
          energy[(i * pic[2].width) + j] +=
              energyCalculator(pic[2].img[(i * pic[2].width) + j - 1],
                               pic[2].img[i * pic[2].width],
                               pic[2].img[(i - 1) * pic[2].width + j],
                               pic[2].img[(i + 1) * pic[2].width + j]);
          energy[(i * pic[2].width) + j] += accumulatedEnergy(
              energy, pic[2].width, auxWidth, (i * pic[2].width) + j);
          ;

        }

        else {
          energy[(i * pic[2].width) + j] +=
              energyCalculator(pic[2].img[(i * pic[2].width) + j - 1],
                               pic[2].img[(i * pic[2].width) + j + 1],
                               pic[2].img[(i - 1) * pic[2].width + j],
                               pic[2].img[(i + 1) * pic[2].width + j]);
          energy[(i * pic[2].width) + j] += accumulatedEnergy(
              energy, pic[2].width, auxWidth, (i * pic[2].width) + j);
          ;
        }
      }
    }

    lowestEnergyPixel = pic[2].width * (pic[2].height - 1);

    for (int i = lowestEnergyPixel;
         i < pic[2].width * (pic[2].height - 1) + auxWidth; i++) {
      if (energy[i] < energy[lowestEnergyPixel])
        lowestEnergyPixel = i;
    }

    for (int i = lowestEnergyPixel;;) {
      int column = i % pic[2].width;

      for (int j = 0; j < auxWidth - column; j++) {
        pic[2].img[i + j] = pic[2].img[i + j + 1];
        pic[1].img[i + j] = pic[1].img[i + j + 1];
      }

      pic[2].img[(i - column) + auxWidth - 1].r = 0;
      pic[2].img[(i - column) + auxWidth - 1].g = 0;
      pic[2].img[(i - column) + auxWidth - 1].b = 0;

      if (i - pic[2].width < 0)
        break;

      int lowestPath = i - pic[2].width;

      int lowestEnergy = energy[i - pic[2].width];

      if (i % pic[2].width != 0 &&
          energy[i - pic[2].width - 1] < lowestEnergy) {
        lowestEnergy = energy[i - pic[2].width - 1];
        lowestPath = i - pic[2].width - 1;
      }

      if (i % pic[2].width != (auxWidth - 1) &&
          energy[i - pic[2].width + 1] < lowestEnergy) {
        lowestEnergy = energy[i - pic[2].width + 1];
        lowestPath = i - pic[2].width + 1;
      }

      i = lowestPath;
    }

    free(energy);
    auxWidth--;
  }

  uploadTexture();
  free(maskAux);
}

int energyCalculator(RGB8 previousPixel,
                     RGB8 nextPixel,
                     RGB8 topPixel,
                     RGB8 lowerPixel) {
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

int accumulatedEnergy(int* energy, int maxWidth, int actualWidth, int i) {
  int finalEnergy = energy[i - maxWidth];

  if (i % maxWidth != 0 && energy[i - maxWidth - 1] < finalEnergy)
    finalEnergy = energy[i - maxWidth - 1];

  if (i % maxWidth != (actualWidth - 1) && energy[i - maxWidth * 1] < finalEnergy) 
    finalEnergy = energy[i - maxWidth * 1];
  
  return finalEnergy;
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
