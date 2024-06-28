// this plugin fetches frames from a ZWO ASI camera
#ifndef AYLP_ASI_H_
#define AYLP_ASI_H_

#include "anyloop.h"

struct aylp_asi_data {
	int cam_index;
	int cam_count;
	int roi_start_y; int roi_start_x;
	int roi_height; int roi_width;
	double pitch_y; double pitch_x;
	int bin;	// binning method
	long exposure_us;
	long gain;
	long bandwidth_overload;
	long high_speed_mode;
	long wb_b;
	long wb_r;
	ASI_IMG_TYPE img_type;
	ASI_CAMERA_INFO cam_info;
	gsl_matrix_uchar *fb;
};

// initialize asi device
int aylp_asi_init(struct aylp_device *self);

// busy-wait for frame
int aylp_asi_process(struct aylp_device *self, struct aylp_state *state);

// close asi device when loop exits
int aylp_asi_close(struct aylp_device *self);

#endif

