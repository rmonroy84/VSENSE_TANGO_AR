#ifndef VSENSE_COMMON_STATUS_H_
#define VSENSE_COMMON_STATUS_H_

#include <string>
#include <sstream>

namespace vsense { namespace common {

class Status {
public:
  enum State {
    Idle = 0,
    RecordFrame,
    LoadPC,
    CreateOctree,
    StorePoints,
    ConsolidateOctree,
    RenderTexture
  };

  Status() {
    state_ = Idle;
    changed_ = false;
    progress_ = 0;
    nbrFrames_ = 0;

	updateString();
  }

  void updateState(State state) {
    state_ = state;
    progress_ = 0;
    changed_ = true;
  }

  void updateFrameCount(unsigned int frame) {
    nbrFrames_ = frame;
    changed_ = true;
  }

  unsigned int incrementFrameCount() {
    nbrFrames_++;
    changed_ = true;
    return nbrFrames_;
  }

  void updateProgress(unsigned char progress) {
    progress_ = progress;
    changed_ = true;
  }

  const std::string& asString() {
    if(changed_)
      updateString();

    return msg_;
  }

private:
  void updateString() {
    std::ostringstream ss;
    ss << "State: ";

    switch(state_){
      case Idle:
        ss << "Idle" << std::endl;
        break;
      case RecordFrame:
        ss << "Recording frames" << std::endl;
        break;
      case LoadPC:
        ss << "Loading PCs" << std::endl;
        break;
      case CreateOctree:
        ss << "Creating octree" << std::endl;
        break;
      case StorePoints:
        ss << "Storing points" << std::endl;
        break;
      case ConsolidateOctree:
        ss << "Consolidating octree" << std::endl;
        break;
      case RenderTexture:
        ss << "Rendering texture" << std::endl;
        break;
    }

    if(state_ != Idle)
      ss << "Progress: " << (int)progress_ << "%" << std::endl;

    ss << "Saved frames: " << nbrFrames_;
    msg_ = ss.str();
  }

  bool changed_;
  State state_;
  unsigned char progress_;
  unsigned int nbrFrames_;

  std::string msg_;
};

} }

#endif