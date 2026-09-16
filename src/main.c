/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pwm.h>
#include "tm1638.h"	
#include <stdlib.h>
#include <math.h>
#include "megalovania.h"

#define USER_NODE DT_PATH(zephyr_user)

#define STACK_SIZE 512

#define PRIORITY_ONE 1
#define PRIORITY_TWO 2
#define PRIORITY_TRE 3
#define PRIORITY_FOR 4
#define PRIORITY_FIV 5

#define CLAVE_CAJA 2707

LOG_MODULE_REGISTER(puerta, LOG_LEVEL_DBG);

//colas

//colas adc

K_MSGQ_DEFINE(lec_raw , sizeof(int16_t),2, 1);
K_MSGQ_DEFINE(lec_fun , sizeof(int16_t),2, 1);
K_MSGQ_DEFINE(sum_fun , sizeof(int),1, 1);
K_MSGQ_DEFINE(lec_fin , sizeof(int16_t),1, 1);

//colas boton

K_MSGQ_DEFINE(botones , sizeof(int),1, 1);
K_MSGQ_DEFINE(antireb, sizeof(int),1, 1);
K_MSGQ_DEFINE(ingreso , sizeof(int),1, 1);
K_MSGQ_DEFINE(numero , sizeof(int),1, 1);

//colas de control

K_MSGQ_DEFINE(open_close , sizeof(bool),1, 1);
K_MSGQ_DEFINE(fail_pass , sizeof(bool),1, 1);
K_MSGQ_DEFINE(s0_s90 , sizeof(bool),1, 1);
K_MSGQ_DEFINE(paspas , sizeof(bool),1, 1);
//semaforos

K_SEM_DEFINE(dis,0,1);
K_SEM_DEFINE(enter,0,1);

K_SEM_DEFINE(bocina,0,1);
K_SEM_DEFINE(pasos,0,1);

//mutex

K_MUTEX_DEFINE(hw_access);

//bobinas

static const struct gpio_dt_spec b[4] ={
	GPIO_DT_SPEC_GET_BY_IDX(USER_NODE, paso_paso_gpios, 0),
	GPIO_DT_SPEC_GET_BY_IDX(USER_NODE, paso_paso_gpios, 1),
	GPIO_DT_SPEC_GET_BY_IDX(USER_NODE, paso_paso_gpios, 2),
	GPIO_DT_SPEC_GET_BY_IDX(USER_NODE, paso_paso_gpios, 3),
};

//tm1638

static const struct gpio_dt_spec stb_pin =
	GPIO_DT_SPEC_GET(USER_NODE, tm1638_stb_gpios);
static const struct gpio_dt_spec clk_pin =
	GPIO_DT_SPEC_GET(USER_NODE, tm1638_clk_gpios);
static const struct gpio_dt_spec dio_pin =
	GPIO_DT_SPEC_GET(USER_NODE, tm1638_dio_gpios);

//ADC

static const struct adc_dt_spec vol =
	ADC_DT_SPEC_GET_BY_NAME(USER_NODE, volumen);

//pwms

static const struct pwm_dt_spec boc = 
	PWM_DT_SPEC_GET(DT_NODELABEL(bocina));
static const struct pwm_dt_spec ser = 
	PWM_DT_SPEC_GET(DT_NODELABEL(servo));

void pasfal(bool i){
	if (i) {
		tm1638_set_digit(4, 0x73); //P
		tm1638_set_digit(5, 0x77); //A
		tm1638_set_digit(6, 0x6D); //S
		tm1638_set_digit(7, 0xED); //S.
	}
	else{
		tm1638_set_digit(4, 0x71); //F
		tm1638_set_digit(5, 0x77); //A
		tm1638_set_digit(6, 0x30); //I
		tm1638_set_digit(7, 0xB8); //L
	}
}

void opco (bool i){
	if (i){
		tm1638_set_digit(4, 0x3F); //O
		tm1638_set_digit(5, 0x73); //P
		tm1638_set_digit(6, 0x79); //E
		tm1638_set_digit(7, 0xD4); //N.
	}
	else{
		tm1638_set_digit(4, 0x39); //C
		tm1638_set_digit(5, 0x38); //L
		tm1638_set_digit(6, 0x3F); //0
		tm1638_set_digit(7, 0xED); //S.
	}
}

//lectura adc

void leer_vol(void){
	int rec;

	int16_t rawvol;

	if (!device_is_ready(vol.dev)){
		LOG_WRN("ADC no esta listo");
	}
	rec = adc_channel_setup_dt(&vol);
	if (rec!=0){
		LOG_WRN("ADC no se pudo configurar");
	}

	struct adc_sequence sec = {
    	.buffer = &rawvol,
    	.buffer_size = sizeof(rawvol),
    };

	while (1){
		rec = adc_sequence_init_dt(&vol, &sec);
		if (rec!=0) LOG_DBG("Error lectura adc: %d", rec);
		rec = adc_read(vol.dev, &sec);
		if (rec!=0) LOG_DBG("Error leer vol: %d", rec);
		k_msgq_put(&lec_raw, &rawvol, K_NO_WAIT);
		k_msleep(50);
	}
}

void funcion_adc (void){
	int16_t rawvol, fin, last = 0;
	float a = 0.1;
	while(1){
		k_msgq_get(&lec_raw, &rawvol, K_FOREVER);
		fin = (int16_t)(last + a*(rawvol - last));
		last = fin;
		k_msgq_put(&lec_fun, &fin, K_NO_WAIT);
	}
}

void suma_adc (void){
	int16_t lec;
	int i = 10, subt=0;
	while (1)
	{
		i ++;
		k_msgq_get(&lec_fun, &lec, K_FOREVER);
		subt += lec;
		if (i == 10){
			i=0;
			k_msgq_put(&sum_fun, &subt, K_NO_WAIT);
			subt = 0;
		}
	}
}

void promedio_adc (void){
	int sum;
	int16_t prom;
	while (1)
	{
		k_msgq_get(&sum_fun, &sum, K_FOREVER);
		prom = sum/10;
		k_msgq_purge(&lec_fin);
		k_msgq_put(&lec_fin, &prom, K_NO_WAIT);
	}
}

//lectura de botones

void lecbot(void){
	int rec, boton;

	rec = tm1638_init(stb_pin, clk_pin, dio_pin);
	if(rec!=0) LOG_WRN("error al iniciar el tm1638 rec = %d\n", rec);

	tm1638_display(0);

	k_msgq_put(&open_close, false, K_NO_WAIT);

	k_sem_give(&dis);
	while(1){
		k_mutex_lock(&hw_access, K_FOREVER);
		boton = tm1638_get_button();
		k_mutex_unlock(&hw_access);
		k_msgq_put(&botones, &boton, K_NO_WAIT);
		k_msleep(50);
	}
}

void antir(void){
	int boton=0, last = 0;
	while(1){
		k_msgq_get(&botones, &boton, K_FOREVER);
		if (boton > 0 && boton != last) {
			k_msgq_put(&antireb, &boton, K_NO_WAIT);
		}
		last = boton;
	}		
}

void menu(void){
	int boton;
	bool i =false;
	while(1){
		k_msgq_get(&antireb, &boton, K_FOREVER);
		if (boton >=1 && boton <= 4) k_msgq_put(&numero, &boton, K_NO_WAIT);
		else if (boton == 5) k_sem_give(&enter);
		else if (boton == 6) k_msgq_put(&open_close, &i, K_NO_WAIT);//boton la cierra nuevamente
	}
}

//display

void numero_mostrar(void){
	int boton, nu[4] = {0,0,0,0}, clave = 0;
	while(1){
		k_msgq_get(&numero, &boton, K_FOREVER);
		nu[boton-1] = (nu[boton-1] + 1) % 10;
		k_mutex_lock(&hw_access, K_FOREVER);
		tm1638_set_number(boton-1, nu[boton-1]);
		k_mutex_unlock(&hw_access);
		clave = nu[0]*1000 + nu[1]*100 + nu[2]*10 + nu[3];
		k_msgq_purge(&ingreso);
		k_msgq_put(&ingreso, &clave, K_NO_WAIT);
	}
}

//interno

void claverec(void){
	int clave= 0 ;
	bool i = true, j =false;
	while(1){
		k_sem_take(&enter, K_FOREVER);
		k_msgq_peek(&ingreso, &clave);
		if (clave == CLAVE_CAJA){
			k_msgq_put(&fail_pass, &i, K_NO_WAIT);
			k_msleep(1000);
			k_msgq_put(&open_close, &i, K_NO_WAIT);
		}
		else{
			k_msgq_put(&fail_pass, &j, K_NO_WAIT); 
		}
	}
}

void openclose(void){
	bool rec, i=true, j=false;
	while(1){
		k_msgq_get(&open_close, &rec, K_FOREVER);
		k_mutex_lock(&hw_access,K_FOREVER);
		opco(rec);
		k_mutex_unlock(&hw_access);
		if (rec == true){
			k_msgq_put(&s0_s90, &i, K_NO_WAIT);
			k_msgq_put(&paspas, &i, K_NO_WAIT);
			k_sem_give(&bocina);
		}
		else{
			k_msgq_put(&s0_s90, &j, K_NO_WAIT);
			k_msgq_put(&paspas, &j, K_NO_WAIT);
		}
	}
}

void failpass(void){
	bool rec;
	while(1){
		k_msgq_get(&fail_pass, &rec, K_FOREVER);
		k_mutex_lock(&hw_access, K_FOREVER);
			pasfal(rec);
		k_mutex_unlock(&hw_access);
	}
}


//funcion de la bocina

void reproducir_nota(const struct pwm_dt_spec *pwm, uint32_t frecuencia_hz, uint32_t duracion_ms)
{
	int16_t rawvol;

    if (frecuencia_hz == SILENCIO)  pwm_set_pulse_dt(pwm, 0);
    else {
		k_msgq_peek(&lec_fin,&rawvol);
        uint32_t periodo_ns = 1000000000U / frecuencia_hz;
        
        uint32_t pulso_max = periodo_ns / 2;

		uint32_t pulso_ns = (pulso_max*rawvol)/4095;

        pwm_set_dt(pwm, periodo_ns, pulso_ns);
    }

    k_msleep(duracion_ms);

    pwm_set_pulse_dt(pwm, 0);
    k_msleep(20);
}

void megalo(void){
	if (!pwm_is_ready_dt(&boc)) LOG_ERR("Bocina no esta lista:");
	while(1){
		k_sem_take(&bocina, K_FOREVER);
		for (int i = 0; i < megalovania_length; i++) {
			reproducir_nota(&boc, megalovania_intro[i].frecuencia, megalovania_intro[i].duracion);
		}
	}
}

//funsiones servo

void servomotor (void){
	if (!pwm_is_ready_dt(&ser)) LOG_ERR("No se pudo configurar el servo");
	pwm_set_pulse_dt(&ser, 1000000);
	bool i;
	while (1){
		k_msgq_get(&s0_s90, &i, K_FOREVER);
		if(i) pwm_set_pulse_dt(&ser, 1000000);
		else pwm_set_pulse_dt(&ser, 2000000);
	}
}

//funciones paso a paso

void pasoapas(void){
	for (int i = 0; i < 4; i++) {
        if (!gpio_is_ready_dt(&b[i])) {
            LOG_ERR("no esta listo el paso a paso");
			return;
        }
        gpio_pin_configure_dt(&b[i], GPIO_OUTPUT_INACTIVE);
    }
	bool k;
	int pasos[8][4]={
		{1,0,0,0},
		{1,1,0,0},
		{0,1,0,0},
		{0,1,1,0},
		{0,0,1,0},
		{0,0,1,1},
		{0,0,0,1},
		{1,0,0,1}};
	while (1)
	{
		k_msgq_get(&paspas, &k, K_FOREVER);
		if (k) {
			for (int i = 0; i < 20; i++)
			{
				for (int j = 0; j < 4; j++)
				{
					gpio_pin_set_dt(&b[j], pasos[i%8][j]);
				}
				k_msleep(20);
			}
		}
		else{
			for (int i = 19; i >= 0; i--)
			{
				for (int j = 0; j < 4; j++)
				{
					gpio_pin_set_dt(&b[j], pasos[i%8][j]);
				}
				k_msleep(20);
			}
		}
	}	
}


K_THREAD_DEFINE (lectura_voltaje, STACK_SIZE, leer_vol, NULL, NULL, NULL,
	PRIORITY_ONE, 0, 0);
K_THREAD_DEFINE (funcion_voltaje, STACK_SIZE, funcion_adc, NULL, NULL, NULL,
	PRIORITY_ONE, 0, 0);
K_THREAD_DEFINE (funcion_suma, STACK_SIZE, suma_adc, NULL, NULL, NULL,
	PRIORITY_ONE, 0, 0);
K_THREAD_DEFINE (promedio_suma, STACK_SIZE, promedio_adc, NULL, NULL, NULL,
	PRIORITY_ONE, 0, 0);

K_THREAD_DEFINE (lectura_boton, STACK_SIZE, lecbot, NULL, NULL, NULL,
	PRIORITY_FIV, 0, 0);
K_THREAD_DEFINE (anti_rebote, STACK_SIZE, antir, NULL, NULL, NULL,
	PRIORITY_FIV, 0, 0);
K_THREAD_DEFINE (menu_boton, STACK_SIZE, menu, NULL, NULL, NULL,
	PRIORITY_FIV, 0, 0);


K_THREAD_DEFINE (displaym, STACK_SIZE, numero_mostrar, NULL, NULL, NULL,
	PRIORITY_FOR, 0, 0);

K_THREAD_DEFINE (rec_clave, STACK_SIZE, claverec, NULL, NULL, NULL,
	PRIORITY_TRE, 0, 0);
K_THREAD_DEFINE (abrir_cerrar, STACK_SIZE, openclose, NULL, NULL, NULL,
	PRIORITY_TRE, 0, 0);
K_THREAD_DEFINE (fallar_pasar, STACK_SIZE, failpass, NULL, NULL, NULL,
	PRIORITY_TRE, 0, 0);

K_THREAD_DEFINE (bocina_megalo, STACK_SIZE, megalo, NULL, NULL, NULL,
	PRIORITY_TWO, 0, 0);

K_THREAD_DEFINE (motor_pasos, STACK_SIZE, pasoapas, NULL, NULL, NULL,
	PRIORITY_TRE, 0, 0);
K_THREAD_DEFINE (motor_servo, STACK_SIZE, servomotor, NULL, NULL, NULL,
	PRIORITY_TRE, 0, 0);