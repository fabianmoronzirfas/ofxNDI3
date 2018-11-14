#include "ofxNDIGrabber3.h"

ofxNDIGrabber3::ofxNDIGrabber3()
{
  _finder = NULL;
  _receiver = NULL;
  _sources = NULL;
  _bandWidth = NDIlib_recv_bandwidth_highest;
  
  _initialized = NDIlib_initialize();
  if (!_initialized)
  {
    ofLogFatalError("ofxNDIGrabber3")<< "cannot run NDI";
    ofLogNotice("ofxNDI")<< "Most likely because the CPU is not sufficient (see SDK documentation). You can check this directly with a call to NDIlib_is_supported_CPU()";
  }
  
  createFinder();
  findSources();
  createReceiver();
}

ofxNDIGrabber3::~ofxNDIGrabber3(){
  if(_receiver) NDIlib_recv_destroy(_receiver);
  if(_finder) NDIlib_find_destroy(_finder);
  if(isInitialized())	NDIlib_destroy();
}
bool ofxNDIGrabber3::setup(int w, int h){
  ofLogNotice("ofxNDIGrabber3") << "ofxNDIGrabber3::setup has no effect";
  return true;
}

void ofxNDIGrabber3::update(){
  if(!isInitialized()) return;
  {	// The descriptors
    NDIlib_video_frame_v2_t video_frame;
    NDIlib_audio_frame_v2_t audio_frame;
    NDIlib_metadata_frame_t metadata_frame;
    
    switch (NDIlib_recv_capture_v2(_receiver, &video_frame, &audio_frame, &metadata_frame, 0))
    {
      // No data
      case NDIlib_frame_type_none:
      ofLogVerbose("ofxNDIGrabber") << "No data received";
      break;
      case NDIlib_frame_type_error:
      break;
      // Video data
      case NDIlib_frame_type_video:
      _newFrame = true;
      ofLogVerbose("ofxNDIGrabber") << "Video data received "<< video_frame.xres << ", "<< video_frame.yres;
      _pixels.clear();
      if(!_pixels.isAllocated()){
        _pixels.allocate(video_frame.xres, video_frame.yres, 4);
      }
      for(int y = 0; y < video_frame.yres; y++){
        for(int x = 0; x < video_frame.xres-3; x++){
          auto index = (x*4 + y*_pixels.getWidth()*4);
          _pixels.setColor(x,y,
                           ofColor(video_frame.p_data[index+2],
                                   video_frame.p_data[index+1],
                                   video_frame.p_data[index]));
          
          
          
        }
      }
      //            ofLogNotice("pixels")<<"width "<<_pixels.getWidth()<<" height "<<_pixels.getHeight();
      NDIlib_recv_free_video_v2(_receiver, &video_frame);
      break;
      
      // Audio data
      case NDIlib_frame_type_audio:
      printf("Audio data received (%d samples).\n", audio_frame.no_samples);
      NDIlib_recv_free_audio_v2(_receiver, &audio_frame);
      break;
      
      // Meta data
      case NDIlib_frame_type_metadata:
      printf("Meta data received.\n");
      NDIlib_recv_free_metadata(_receiver, &metadata_frame);
      break;
    }
  }
  _image.setFromPixels(_pixels);
}

void ofxNDIGrabber3::draw(float x, float y) const {
  if(_image.isAllocated()){
    _image.draw(x,y);
  }
}

void ofxNDIGrabber3::draw(float x, float y, float width, float height) const {
  if(_image.isAllocated()){
    _image.draw(x, y, width, height);
  }
}

void ofxNDIGrabber3::close(){
  if(_receiver){
    NDIlib_recv_destroy(_receiver);
  }
}

void ofxNDIGrabber3::reloadSources(){
  _sources = NULL;
  createFinder();
  findSources();
}
void ofxNDIGrabber3::threadedFunction(){
  while (isThreadRunning()) {
    update();
  }
}

bool ofxNDIGrabber3::isFrameNew() const{
  return _newFrame;
}
bool ofxNDIGrabber3::setPixelFormat(ofPixelFormat pixelFormat){
  ofLogNotice("ofxNDIGrabber")<<"ofxNDIGrabber3::setPixelFormat has no effect";
  return true;
}

bool ofxNDIGrabber3::isInitialized() const{
  return _initialized;
}

float ofxNDIGrabber3::getHeight() const{
  return _pixels.getHeight();
}

float ofxNDIGrabber3::getWidth() const{
  return _pixels.getWidth();
}

ofPixelFormat ofxNDIGrabber3::getPixelFormat() const{
  return _pixels.getPixelFormat();
}

vector<ofVideoDevice> ofxNDIGrabber3::listDevices() const{
  
  vector<ofVideoDevice> devices;
  for(auto i = 0; i < _numberOfSources; i++){
    devices.push_back(ofVideoDevice());
    devices[i].deviceName = _sources[i].p_ndi_name;
    devices[i].id = i;
  }
  return devices;
}
void ofxNDIGrabber3::setDevice(ofVideoDevice device){
  createReceiver(device.id);
}
void ofxNDIGrabber3::setDevice(int id){
  createReceiver(id);
}
void ofxNDIGrabber3::setDevice(string name){
  for(auto i = 0; i < _numberOfSources; i++){
    auto source = _sources[i];
    if(source.p_ndi_name == name){
      createReceiver(i);
    }
  }
}


std::string ofxNDIGrabber3::getNDIVersion(){
  return NDIlib_version();
}

ofPixels &ofxNDIGrabber3::getPixels()
{
  _newFrame = false;
  return _pixels;
}

const ofPixels &ofxNDIGrabber3::getPixels() const
{
  return _pixels;
}

void ofxNDIGrabber3::setLowBandwidth(bool value){
  if(value){
    _bandWidth = NDIlib_recv_bandwidth_lowest;
  }else{
    _bandWidth = NDIlib_recv_bandwidth_highest;
  }
}

bool ofxNDIGrabber3::createFinder(){
  if(_finder){
    NDIlib_find_destroy(_finder);
  }
  
  const NDIlib_find_create_t NDI_find_create_desc = { TRUE, NULL };
  _finder = NDIlib_find_create_v2(&NDI_find_create_desc);
  if(!_finder){
    ofLogError("ofxNDIGrabber3") << "could not create finder";
  }else{
    ofLogNotice("ofxNDIGrabber3") << "successfully created finder";
  }
  
  return _finder;
}
int ofxNDIGrabber3::findSources(){
  uint32_t no_sources = 0;
  auto startTime = ofGetElapsedTimef();
  
  while (true){
    auto currentTime = ofGetElapsedTimef();
    if(currentTime - startTime > 10){
      break;
    }
    if (!NDIlib_find_wait_for_sources(_finder, 1000/* 1 second */))	{
      // printf("No change to the sources found.\n");
      ofLogNotice("ofxNDIGrabber3") << "waiting for sources";

      continue;
      }
    _sources = NDIlib_find_get_current_sources(_finder, &no_sources);
    		for (uint32_t i = 0; i < no_sources; i++){

			printf("%u. %s\n", i + 1, _sources[i].p_ndi_name);
        }
  }
  ofLogNotice("ofxNDIGrabber3") << "found " << no_sources << " sources ";
  _numberOfSources = no_sources;
  return no_sources;
}

bool ofxNDIGrabber3::createReceiver(int id){
  if(id >= _numberOfSources) return false;
  if(_receiver){
    NDIlib_recv_destroy(_receiver);
  }
  
#ifdef TARGET_WIN32
  NDIlib_recv_create_v3_t NDI_recv_create_desc = { _sources[id], NDIlib_recv_color_format_e_RGBX_RGBA, _bandWidth, TRUE };
#endif
#ifdef TARGET_OSX
  NDIlib_recv_create_v3_t NDI_recv_create_desc = { _sources[id], FALSE, _bandWidth, TRUE };
#endif
#ifdef TARGET_LINUX
  NDIlib_recv_create_v3_t NDI_recv_create_desc = { _sources[id], FALSE, _bandWidth, TRUE };
#endif

  _receiver = NDIlib_recv_create_v3(&NDI_recv_create_desc);
  
  if (!_receiver){
    ofLogFatalError("ofxNDIGrabber")<<"cannot create NDI receiver";
  }
  return _receiver;
}
