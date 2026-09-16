#ifndef MEGALOVANIA_H
#define MEGALOVANIA_H

#include <stdint.h>

/* Estructura para definir cada paso de la melodía */
struct nota_musical {
    uint32_t frecuencia; /* Frecuencia en Hz (0 para silencio) */
    uint32_t duracion;   /* Duración en milisegundos */
};

/* Frecuencias de las notas necesarias (en Hz) */
#define NOTA_B3  247
#define NOTA_BB3 233
#define NOTA_C4  262
#define NOTA_D4  294
#define NOTA_F4  349
#define NOTA_G4  392
#define NOTA_GS4 415
#define NOTA_A4  440
#define NOTA_D5  587
#define SILENCIO 0

/* Tempo base en milisegundos */
#define TEMPO 110

/* Partitura de la intro (4 compases) */
static const struct nota_musical megalovania_intro[] = {
    /* Compás 1 */
    {NOTA_D4, TEMPO}, {NOTA_D4, TEMPO}, {NOTA_D5, TEMPO * 2}, {NOTA_A4, TEMPO * 2},
    {SILENCIO, TEMPO}, {NOTA_GS4, TEMPO}, {SILENCIO, TEMPO / 2}, {NOTA_G4, TEMPO * 2},
    {NOTA_F4, TEMPO * 2}, {NOTA_D4, TEMPO}, {NOTA_F4, TEMPO}, {NOTA_G4, TEMPO},

    /* Compás 2 */
    {NOTA_C4, TEMPO}, {NOTA_C4, TEMPO}, {NOTA_D5, TEMPO * 2}, {NOTA_A4, TEMPO * 2},
    {SILENCIO, TEMPO}, {NOTA_GS4, TEMPO}, {SILENCIO, TEMPO / 2}, {NOTA_G4, TEMPO * 2},
    {NOTA_F4, TEMPO * 2}, {NOTA_D4, TEMPO}, {NOTA_F4, TEMPO}, {NOTA_G4, TEMPO},

    /* Compás 3 */
    {NOTA_B3, TEMPO}, {NOTA_B3, TEMPO}, {NOTA_D5, TEMPO * 2}, {NOTA_A4, TEMPO * 2},
    {SILENCIO, TEMPO}, {NOTA_GS4, TEMPO}, {SILENCIO, TEMPO / 2}, {NOTA_G4, TEMPO * 2},
    {NOTA_F4, TEMPO * 2}, {NOTA_D4, TEMPO}, {NOTA_F4, TEMPO}, {NOTA_G4, TEMPO},

    /* Compás 4 */
    {NOTA_BB3, TEMPO}, {NOTA_BB3, TEMPO}, {NOTA_D5, TEMPO * 2}, {NOTA_A4, TEMPO * 2},
    {SILENCIO, TEMPO}, {NOTA_GS4, TEMPO}, {SILENCIO, TEMPO / 2}, {NOTA_G4, TEMPO * 2},
    {NOTA_F4, TEMPO * 2}, {NOTA_D4, TEMPO}, {NOTA_F4, TEMPO}, {NOTA_G4, TEMPO}
};

/* Calculamos automáticamente la cantidad de notas del arreglo */
static const int megalovania_length = sizeof(megalovania_intro) / sizeof(megalovania_intro[0]);

#endif /* MEGALOVANIA_H */