#include "ASICamera2.h"
#include "anyloop.h"
#include "logging.h"
#include "xalloc.h"
#include "aylp_asi.h"


static const int timeout_ms = 500;


static const char *asi_strerror(int err) {
	switch (err) {
	case ASI_ERROR_INVALID_INDEX:
		return "No camera connected or index value out of boundary.";
	case ASI_ERROR_INVALID_ID:
		return "Invalid ID.";
	case ASI_ERROR_INVALID_CONTROL_TYPE:
		return "Invalid control type.";
	case ASI_ERROR_CAMERA_CLOSED:
		return "Camera didn't open.";
	case ASI_ERROR_CAMERA_REMOVED:
		return (
			"Failed to find the camera;"
			"maybe the camera has been removed."
		);
	case ASI_ERROR_INVALID_PATH:
		return "Cannot find the path of the file.";
	case ASI_ERROR_INVALID_FILEFORMAT:
		return "Invalid file format.";
	case ASI_ERROR_INVALID_SIZE:
		return "Wrong video format size.";
	case ASI_ERROR_INVALID_IMGTYPE:
		return "Unsupported image format.";
	case ASI_ERROR_OUTOF_BOUNDARY:
		return "The startpos is out of boundary.";
	case ASI_ERROR_TIMEOUT:
		return "Timeout.";
	case ASI_ERROR_INVALID_SEQUENCE:
		return "Stop capture first.";
	case ASI_ERROR_BUFFER_TOO_SMALL:
		return "Buffer size is not big enough.";
	case ASI_ERROR_VIDEO_MODE_ACTIVE:
		return "Video mode active.";
	case ASI_ERROR_EXPOSURE_IN_PROGRESS:
		return "Exposure in progress.";
	case ASI_ERROR_GENERAL_ERROR:
		return "General error, eg: value is out of valid range.";
	case ASI_ERROR_INVALID_MODE:
		return "Invalid mode.";
	case ASI_ERROR_END:
		return "ASI_ERROR_END";
	default:
		return "No error.";
	}
}


int aylp_asi_init(struct aylp_device *self)
{
	int err;
	self->device_data = xcalloc(1, sizeof(struct aylp_asi_data));
	struct aylp_asi_data *data = self->device_data;
	// attach methods
	self->process = &aylp_asi_process;
	self->close = &aylp_asi_close;

	// default params (-1 means uninitialized for now)
	data->cam_index = -1;
	data->cam_count = 0;
	data->roi_start_x = 0;
	data->roi_start_y = 0;
	data->roi_width = -1;
	data->roi_height = -1;
	data->pitch_y = 0;
	data->pitch_x = 0;
	data->bin = 1;			// TODO: json
	data->img_type = ASI_IMG_RAW8;	// TODO: json
	data->exposure_us = 10000;
	data->gain = 0;
	data->bandwidth_overload = 40;
	data->high_speed_mode = 0;
	data->wb_b = 90;
	data->wb_r = 48;
	// TODO: document parameters in .md file
	// parse the params json into our data struct
	if (!self->params) {
		log_error("No params object found.");
		return -1;
	}
	json_object_object_foreach(self->params, key, val) {
		if (key[0] == '_') {
			// keys starting with _ are comments
		} else if (!strcmp(key, "cam_index")) {
			data->cam_index = json_object_get_int(val);
			log_trace("cam_index = %d", data->cam_index);
		} else if (!strcmp(key, "roi_start_y")) {
			data->roi_start_y = json_object_get_int(val);
			log_trace("roi_start_y = %d", data->roi_width);
		} else if (!strcmp(key, "roi_start_x")) {
			data->roi_start_x = json_object_get_int(val);
			log_trace("roi_start_x = %d", data->roi_start_x);
		} else if (!strcmp(key, "roi_height")) {
			data->roi_height = json_object_get_int(val);
			log_trace("roi_height = %d", data->roi_height);
		} else if (!strcmp(key, "roi_width")) {
			data->roi_width = json_object_get_int(val);
			log_trace("roi_width = %d", data->roi_width);
		} else if (!strcmp(key, "pitch_y")) {
			data->pitch_y = json_object_get_double(val);
			log_trace("pitch_y = %G", data->pitch_y);
		} else if (!strcmp(key, "pitch_x")) {
			data->pitch_x = json_object_get_double(val);
			log_trace("pitch_x = %G", data->pitch_x);
		} else if (!strcmp(key, "exposure_us")) {
			data->exposure_us = json_object_get_int(val);
			log_trace("exposure_us = %ld", data->exposure_us);
		} else if (!strcmp(key, "gain")) {
			data->gain = json_object_get_int(val);
			log_trace("gain = %ld", data->gain);
		} else if (!strcmp(key, "bandwidth_overload")) {
			data->bandwidth_overload = json_object_get_int(val);
			log_trace("bandwidth_overload = %ld",
				data->bandwidth_overload
			);
		} else if (!strcmp(key, "high_speed_mode")) {
			data->high_speed_mode = json_object_get_int(val);
			log_trace("high_speed_mode = %ld",
				data->high_speed_mode
			);
		} else if (!strcmp(key, "wb_b")) {
			data->wb_b = json_object_get_int(val);
			log_trace("wb_b = %ld", data->wb_b);
		} else if (!strcmp(key, "wb_r")) {
			data->wb_r = json_object_get_int(val);
			log_trace("wb_r = %ld", data->wb_r);
		} else {
			log_warn("Unknown parameter \"%s\"", key);
		}
	}
	// make sure we didn't miss any params
	if (data->cam_index < 0) {
		log_error("You must provide a nonnegative cam_index.");
		return -1;
	}

	data->cam_count = ASIGetNumOfConnectedCameras();
	log_info("Seeing %d ASI cameras", data->cam_count);
	if (data->cam_count < data->cam_index + 1) {
		log_error("Camera count lower than camera index + 1");
		return -1;
	}
	for (int i = 0; i < data->cam_count; i++) {
		ASIGetCameraProperty(&data->cam_info, i);
		log_info("Camera %d: %s",i, data->cam_info.Name);
	}
	ASIGetCameraProperty(&data->cam_info, data->cam_index);

	err = ASIOpenCamera(data->cam_info.CameraID);
	if (err) {
		log_error(
			"ASI error while opening camera (permissions?): %s",
			asi_strerror(err)
		);
		return -1;
	}
	err = ASIInitCamera(data->cam_info.CameraID);
	if (err) {
		log_error(
			"ASI error while initializing camera: %s",
			asi_strerror(err)
		);
		return -1;
	}

	// set more defaults
	if (data->roi_height < 0) data->roi_height = data->cam_info.MaxHeight;
	if (data->roi_width < 0) data->roi_width = data->cam_info.MaxWidth;

	log_trace("Setting ROI with height=%d,width=%d starting at y=%d,x=%d",
		data->roi_height, data->roi_width,
		data->roi_start_y, data->roi_start_x
	);
	err = ASISetROIFormat(data->cam_info.CameraID,
		data->roi_width, data->roi_height, data->bin, data->img_type
	);
	if (err) {
		log_error("Failed to set ROI format: %s", asi_strerror(err));
		return -1;
	}
	err = ASISetStartPos(data->cam_info.CameraID,
		data->roi_start_x, data->roi_start_y
	);
	if (err) {
		log_error("Failed to set ROI start: %s", asi_strerror(err));
		return -1;
	}

	// allocate framebuffer
	data->fb = xcalloc_type(gsl_matrix_uchar,
		data->roi_height, data->roi_width
	);

	// set camera controls
	ASI_CONTROL_TYPE controls[] = {
		ASI_EXPOSURE, ASI_GAIN, ASI_BANDWIDTHOVERLOAD,
		ASI_HIGH_SPEED_MODE, ASI_WB_B, ASI_WB_R
	};
	long values[] = {
		data->exposure_us, data->gain, data->bandwidth_overload,
		data->high_speed_mode, data->wb_b, data->wb_r
	};
	ASI_BOOL autos[] = {
		ASI_FALSE, ASI_FALSE, ASI_FALSE,
		ASI_FALSE, ASI_FALSE, ASI_TRUE
	};
	for (size_t i = 0; i < sizeof(controls)/sizeof(controls[0]); i++) {
		log_trace("Setting control %d to %ld", controls[i], values[i]);
		err = ASISetControlValue(data->cam_info.CameraID,
			controls[i], values[i], autos[i]
		);
		if (err) {
			log_error("Couldn't set control value %d to %ld: %s",
				controls[i], values[i], asi_strerror(err)
			);
			return -1;
		}
	}

	// start capture and print sensor temp
	ASIStartVideoCapture(data->cam_info.CameraID);
	long temp;
	ASI_BOOL b;
	ASIGetControlValue(data->cam_info.CameraID, ASI_TEMPERATURE, &temp, &b);
	log_info("Sensor temperature: %.1f °C", temp/10.0);

	// set types and units
	self->type_in = AYLP_T_ANY;
	self->units_in = AYLP_U_ANY;
	self->type_out = AYLP_T_MATRIX_UCHAR;
	self->units_out = AYLP_U_COUNTS;
	return 0;
}


int aylp_asi_process(struct aylp_device *self, struct aylp_state *state)
{
	// They recommend we grab frames from another thread but I don't really
	// see how that helps us here
	int err;
	struct aylp_asi_data *data = self->device_data;

	err = ASIGetVideoData(data->cam_info.CameraID, data->fb->data,
		data->fb->size1 * data->fb->size2, timeout_ms
	);
	if (err) {
		log_error("Error while getting video data: %s",
			strerror(err)
		);
	}

	// zero-copy update of pipeline state
	state->matrix_uchar = data->fb;
	// housekeeping on the header
	state->header.type = self->type_out;
	state->header.units = self->units_out;
	state->header.log_dim.y = data->fb->size1;
	state->header.log_dim.x = data->fb->size2;
	state->header.pitch.y = data->pitch_y;
	state->header.pitch.x = data->pitch_x;
	return 0;
}


int aylp_asi_close(struct aylp_device *self)
{
	struct aylp_asi_data *data = self->device_data;
	ASIStopVideoCapture(data->cam_info.CameraID);
	ASICloseCamera(data->cam_info.CameraID);
	xfree(data);
	return 0;
}

