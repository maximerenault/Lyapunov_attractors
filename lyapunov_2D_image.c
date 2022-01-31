#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include "bmp.h"

#define MAX(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })

#define MIN(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })

#define ABS(a) ({ __typeof__ (a) _a = (a); _a < 0 ? -_a : _a; })

int maxiter = 3000000;
int n_max = 30000;
double *x,*y;

void SaveAttractor(int, double *, double *, double, double, double, double);

int main(void)
{
	double ax[6], ay[6], stats[5];
	int i,n;
	long secs;
	double xmin=1e32, xmax=-1e32, ymin=1e32, ymax=-1e32, xmin2=1e32, xmax2=-1e32, ymin2=1e32, ymax2=-1e32;
	double d0, dd, dx, dy, lyapunov;
	double xe, ye, xenew, yenew;
	bool valid;

	x = malloc(maxiter*sizeof(double));
	y = malloc(maxiter*sizeof(double));
	time(&secs);
	srand(secs);

	for (n=0;n<n_max;n++) {
		printf("Test num %4d : ",n);

		/* Initialise things for this attractor */
		for (i=0;i<6;i++) {
			ax[i] = 4 * ((double)rand() / (double)RAND_MAX - 0.5);
			ay[i] = 4 * ((double)rand() / (double)RAND_MAX - 0.5);
		}
		lyapunov = 0;
		xmin =  1e32;
		xmax = -1e32;
		ymin =  1e32;
		ymax = -1e32;
	
		/* Calculate the attractor */
		valid = true;
		x[0] = (double)rand() / (double)RAND_MAX - 0.5;
		y[0] = (double)rand() / (double)RAND_MAX - 0.5;
		do {
			xe = x[0] + ((double)rand() / (double)RAND_MAX - 0.5) / 1000.0;
      		ye = y[0] + ((double)rand() / (double)RAND_MAX - 0.5) / 1000.0;
			dx = x[0] - xe;
			dy = y[0] - ye;
			d0 = sqrt(dx * dx + dy * dy);
		} while (d0 <= 0);

		for (i=1;i<maxiter;i++) {

			/* Calculate next term */
      		x[i] = 	ax[0] + ax[1]*x[i-1] + ax[2]*x[i-1]*x[i-1] + 
					ax[3]*x[i-1]*y[i-1] + ax[4]*y[i-1] + ax[5]*y[i-1]*y[i-1];
      		y[i] = 	ay[0] + ay[1]*x[i-1] + ay[2]*x[i-1]*x[i-1] + 
					ay[3]*x[i-1]*y[i-1] + ay[4]*y[i-1] + ay[5]*y[i-1]*y[i-1];
			xenew = ax[0] + ax[1]*xe + ax[2]*xe*xe +
					ax[3]*xe*ye + ax[4]*ye + ax[5]*ye*ye;
			yenew = ay[0] + ay[1]*xe + ay[2]*xe*xe +
					ay[3]*xe*ye + ay[4]*ye + ay[5]*ye*ye;

			/* Update the bounds for image scale */
			xmin = MIN(xmin,x[i]);
			ymin = MIN(ymin,y[i]);
			xmax = MAX(xmax,x[i]);
			ymax = MAX(ymax,y[i]);
			if (i>1000) {
				xmin2 = MIN(xmin,x[i]);
				ymin2 = MIN(ymin,y[i]);
				xmax2 = MAX(xmax,x[i]);
				ymax2 = MAX(ymax,y[i]);
			}

			/* Does the series tend to infinity */
			if (xmin < -1e10 || ymin < -1e10 || xmax > 1e10 || ymax > 1e10) {
				valid = false;
				printf("infinite attractor ");
				stats[0] += 100./n_max;
				break;
			}

			/* Does the series tend to a point */
			dx = x[i] - x[i-1];
			dy = y[i] - y[i-1];
			if (ABS(dx) < 1e-10 && ABS(dy) < 1e-10) {
				valid = false;
				printf("point attractor ");
				stats[1] += 100./n_max;
				break;
			}

			/* Calculate the lyapunov exponents */
			if (i > 1000) {
				dx = x[i] - xenew;
				dy = y[i] - yenew;
				dd = sqrt(dx * dx + dy * dy);
				lyapunov += log(fabs(dd / d0));
				xe = x[i] + d0 * dx / dd;
				ye = y[i] + d0 * dy / dd;
			}
		}

		/* Classify the series according to lyapunov */
		if (valid) {
			if (ABS(lyapunov) < 10) {
				printf("neutrally stable ");
				stats[2] += 100./n_max;
				valid = false;
			} else if (lyapunov < 0) {
				printf("periodic ");
				stats[3] += 100./n_max;
				valid = false; 
			} else {
				printf("chaotic. ");
				stats[4] += 100./n_max;
			}
		}

		/* Save the image */
		if (valid) 
			SaveAttractor(n,ax,ay,xmin2,xmax2,ymin2,ymax2);

		printf("\n");
	}

	/* Statistics */
	printf("\n%2.3f%% infinite attractor\n%2.3f%% point attractor\n%2.3f%% neutrally stable\n%2.3f%% periodic\n%2.3f%% chaotic\n",
			stats[0], stats[1], stats[2], stats[3], stats[4]);

	return 0;
}

void SaveAttractor(int n, double *a, double *b, double xmin, double xmax, double ymin, double ymax) {
	char fname[600];
	int i,j,im_x,im_y;
	Bitmap *image = NULL;
	int width = 2000, height = 2000;

	printf("Saving... ");

	/* Save the parameters */
	sprintf(fname,"x_%f_y_%f_ax_%f_%f_%f_%f_%f_%f_ay_%f_%f_%f_%f_%f_%f.png",
			x[0],y[0],a[0],a[1],a[2],a[3],a[4],a[5],b[0],b[1],b[2],b[3],b[4],b[5]); 

	/* Save the image */
	image = bm_create(width, height);
	
    for (i=0;i<width;i++) {
		for (j=0;j<height;j++){
			bm_set(image,i,j,bm_atoi("black"));
		}
	}

	for (i=1000;i<maxiter;i++) {
		im_x = (width-1) * (x[i] - xmin) / (xmax - xmin);
		im_y = (height-1) * (y[i] - ymin) / (ymax - ymin);
		bm_set(image, im_x, im_y, bm_hsl(60*(i-1000)/(maxiter-1000), 90, 50+20*(i-1000)/(maxiter-1000)));
	}

	bm_save(image,fname);
	bm_free(image);
}
