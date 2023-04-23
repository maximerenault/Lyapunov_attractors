#include <iostream>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <cstring>
#include "jpeg.cpp"
#include "mux.cpp"

#define STREAM_BITRATE      2000000
#define MAXITERATIONS 		1000000
#define NEXAMPLES 			2000
#define NFRAMES 			1000

double *x = new double[MAXITERATIONS];
double *y = new double[MAXITERATIONS];
static int width = 1280, height = 720;

void SaveAttractor(bool, int, double *, double *, double, double, double, double);

int main(int argc,char **argv)
{
	double ax[6],ay[6];
	double factx[6],facty[6];
	std::fill_n(factx, 6, 1.00015);std::fill_n(facty, 6, 1.00015);
	double ax_prev[6],ay_prev[6];
	int i,n,c=0;
	bool drawit;
	long secs;
	double xmin=1e32,xmax=-1e32,ymin=1e32,ymax=-1e32,xmin2=1e32,xmax2=-1e32,ymin2=1e32,ymax2=-1e32;
	double d0,dd,dx,dy,lyapunov;
	double xe,ye,xenew,yenew,a = 32768.0;

	bool video = false;

	if (argc < 2) {
        printf("usage: %s video/image\n"
               "This program generates images from Lyapunov chaotic series\n"
               "and saves them to jpeg files or an mp4 video.\n"
               "With image, random images are generated.\n"
               "With video, a continuous flow of images is generated.\n"
               "\n", argv[0]);
        return 1;
    }

	if (std::string(argv[1]) == "video"){
		video = true;
		prepare_mp4(width, height, STREAM_BITRATE);
	}

	time(&secs);
	srand(secs);

	for (n=0;n<NEXAMPLES;n++) {
		// Initialise things for this attractor
		if(c==0 || !video){
			for (i=0;i<6;i++) {
				ax[i] = 4 * ((double)rand() / (double)RAND_MAX - 0.5);
				ay[i] = 4 * ((double)rand() / (double)RAND_MAX - 0.5);
			}
		}
		else{
			if (true){
				for (i=0;i<6;i++) {
					ax[i] = ax[i]*factx[i];
					ay[i] = ay[i]*facty[i];
					ax_prev[i] = ax[i];
					ay_prev[i] = ay[i];
				}
			}
			else {
				for (i=0;i<6;i++) {
					factx[i] = 1. + 0.0006 * ((double)rand() / (double)RAND_MAX - 0.5);
					facty[i] = 1. + 0.0006 * ((double)rand() / (double)RAND_MAX - 0.5);
					ax[i] = ax_prev[i]*factx[i];
					ay[i] = ay_prev[i]*facty[i];
				}
			}
		}

		lyapunov = 0;
		xmin =  1e32; xmin2 =  1e32;
		xmax = -1e32; xmax2 = -1e32;
		ymin =  1e32; ymin2 =  1e32;
		ymax = -1e32; ymax2 = -1e32;
	
		// Calculate the attractor
		drawit = true;
		if(c==0 || !video){
			x[0] = (double)rand() / (double)RAND_MAX - 0.5;
			y[0] = (double)rand() / (double)RAND_MAX - 0.5;
			do {
				xe = x[0] + ((double)rand() / (double)RAND_MAX - 0.5) / 1000.0;
				ye = y[0] + ((double)rand() / (double)RAND_MAX - 0.5) / 1000.0;
				dx = x[0] - xe;
				dy = y[0] - ye;
				d0 = sqrt(dx * dx + dy * dy);
			} while (d0 <= 0);
		}

		for (i=1;i<MAXITERATIONS;i++) {

			// Calculate next term
      		x[i] = ax[0] + ax[1]*x[i-1] + ax[2]*x[i-1]*x[i-1] + 
				   ax[3]*x[i-1]*y[i-1] + ax[4]*y[i-1] + ax[5]*y[i-1]*y[i-1];
      		y[i] = ay[0] + ay[1]*x[i-1] + ay[2]*x[i-1]*x[i-1] + 
				   ay[3]*x[i-1]*y[i-1] + ay[4]*y[i-1] + ay[5]*y[i-1]*y[i-1];
			xenew = ax[0] + ax[1]*xe + ax[2]*xe*xe +
					ax[3]*xe*ye + ax[4]*ye + ax[5]*ye*ye;
			yenew = ay[0] + ay[1]*xe + ay[2]*xe*xe +
					ay[3]*xe*ye + ay[4]*ye + ay[5]*ye*ye;

			// Update the bounds
			xmin = std::min(xmin,x[i]);
			ymin = std::min(ymin,y[i]);
			xmax = std::max(xmax,x[i]);
			ymax = std::max(ymax,y[i]);
			if (i > (int)(0.5*MAXITERATIONS)) {
				xmin2 = std::min(xmin2,x[i]);
				ymin2 = std::min(ymin2,y[i]);
				xmax2 = std::max(xmax2,x[i]);
				ymax2 = std::max(ymax2,y[i]);
			}

			// Does the series tend to infinity
			if (xmin < -1e10 || ymin < -1e10 || xmax > 1e10 || ymax > 1e10) {
				drawit = false;
				// std::cout << n << " infinite attractor" << std::endl;
				break;
			}

			// Does the series tend to a point
			dx = x[i] - x[i-1];
			dy = y[i] - y[i-1];
			if (abs(dx) < 1e-10 && abs(dy) < 1e-10) {
				drawit = false;
				// std::cout << n << " point attractor" << std::endl;
				break;
			}

			// Calculate the lyapunov exponents
			if (i > (int)(0.5*MAXITERATIONS)) {
				dx = x[i] - xenew;
				dy = y[i] - yenew;
				dd = sqrt(dx * dx + dy * dy);
				lyapunov += log(fabs(dd / d0));
				xe = x[i] + d0 * dx / dd;
				ye = y[i] + d0 * dy / dd;
			}
		}

		// Classify the series according to lyapunov
		if (drawit) {
			if (abs(lyapunov) < 10) {
				// std::cout << n << " neutrally stable" << std::endl;
				drawit = false;
			} else if (lyapunov < 0) {
				// std::cout << n << " periodic " << lyapunov << std::endl;
				drawit = false; 
			} else {
				// std::cout << n << " chaotic " << lyapunov << std::endl;
				c+=1; 
			}
		}

		// Save the image
		if (drawit) SaveAttractor(video, n, ax, ay, xmin2, xmax2, ymin2, ymax2);
		if (c>NFRAMES) break;
	}

	if (video) end_mp4();
	exit(0);
}

void SaveAttractor(bool video, int n, double *a, double *b,
					double xmin, double xmax, double ymin, double ymax) {
	static char fname[600];
	static int ix, iy;
	static unsigned char* pixel_data = new unsigned char[width * height * 3];

	// Save the parameters
	std::sprintf(fname,"%d_x_%f_y_%f_ax_%f_%f_%f_%f_%f_%f_ay_%f_%f_%f_%f_%f_%f.jpg",
				n,x[0],y[0],a[0],a[1],a[2],a[3],a[4],a[5],b[0],b[1],b[2],b[3],b[4],b[5]);

	// Initialize image to black
	memset(pixel_data, 0, width * height * 3 * sizeof(unsigned char));

	for (int i = 100; i < MAXITERATIONS; i++) {
		ix = (width-1) * (x[i] - xmin) / (xmax - xmin);
		iy = (height-1) * (y[i] - ymin) / (ymax - ymin);
		if( ix>(width-1) || ix<0 || iy>(height-1) || iy<0) continue;
		pixel_data[(iy * width + ix) * 3 + 0] = std::min((pixel_data[(iy * width + ix) * 3 + 0] + 1),255) ; // red
		pixel_data[(iy * width + ix) * 3 + 1] = std::min((pixel_data[(iy * width + ix) * 3 + 1] + 10),255) ; // green
		pixel_data[(iy * width + ix) * 3 + 2] = std::min((pixel_data[(iy * width + ix) * 3 + 2] + 40),255) ; // blue
	}

    // Save the image to a JPEG file or MP4 video
	if (video) {
		write_frame_mp4(pixel_data, height, width);
	}
	else {
		write_jpeg_file(fname, pixel_data, height, width, 90);
	}
}
