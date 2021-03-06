#include "flitr/modules/flitr_image_processors/object_detection_CNN/caffe_ssd_functions.h"

#ifdef FLITR_USE_CAFFE
    #include <caffe/caffe.hpp>

#ifdef USE_OPENCV
    #include <opencv2/core/core.hpp>
    #include <opencv2/highgui/highgui.hpp>
    #include <opencv2/imgproc/imgproc.hpp>

using namespace caffe;  // NOLINT(build/namespaces)

using std::cout; using std::endl;

double getElapsedTime(timeval startTime, timeval endTime){
    long seconds  = endTime.tv_sec  - startTime.tv_sec;
    long useconds = endTime.tv_usec - startTime.tv_usec;    //decimal sec

    long milliSec = ((seconds) * 1000 + useconds/1000.0) + 0.5;
    return (double) milliSec/1000;
}



SingleShotDetector::SingleShotDetector(const string& model_file,const string& weights_file,
                   const string& mean_file,
                   const string& mean_value,
                   const string& labels_filename) {



#ifdef CPU_ONLY
  Caffe::set_mode(Caffe::CPU);
#else
  Caffe::set_mode(Caffe::GPU);
#endif


  /* Load the network. */
  net_.reset(new Net<float>(model_file, TEST));
  net_->CopyTrainedLayersFrom(weights_file);

  if(net_->num_inputs() != 1) std::cerr << "Network should have exactly one input." <<endl;
  if( net_->num_outputs() != 1) std::cerr << "Network should have exactly one output." <<endl;

  Blob<float>* input_layer = net_->input_blobs()[0];
  num_channels_ = input_layer->channels();
  CHECK(num_channels_ == 3 || num_channels_ == 1)
    << "Input layer should have 1 or 3 channels.";
  input_geometry_ = cv::Size(input_layer->width(), input_layer->height());

  /* Load the binaryproto mean file. */
#ifndef GFLAGS_GFLAGS_H_
  namespace gflags = google;
#endif

  //Set the Mean file or value
  SetMean(mean_file, mean_value);

  //![ Load class names for the index labels]
  //Read Labels textfile
  string textLine;
  std::ifstream labels_file(labels_filename);

  if(labels_file.is_open()){

      while(std::getline(labels_file,textLine)){
          ClassNames.push_back(textLine);
      }
  }
  labels_file.close();
  cout<<"Considered ..." << ClassNames.size() <<" labels from " <<labels_filename <<endl;



}

std::vector<vector<float> > SingleShotDetector::Detect(const cv::Mat& img) {
  Blob<float>* input_layer = net_->input_blobs()[0];
  input_layer->Reshape(1, num_channels_,
                       input_geometry_.height, input_geometry_.width);
  /* Forward dimension change to all layers. */
  net_->Reshape();

  std::vector<cv::Mat> input_channels;
  WrapInputLayer(&input_channels);

  Preprocess(img, &input_channels);

  //## Fixes the bug of CAFFE running slow, after seting the mode GPU, FPS improved from 0.2fps to >= 30fps
  Caffe::set_mode(Caffe::GPU);

  net_->Forward();

  /* Copy the output layer to a std::vector */
  Blob<float>* result_blob = net_->output_blobs()[0];
  const float* result = result_blob->cpu_data();
  const int num_det = result_blob->height();
  vector<vector<float> > detections;
  for (int k = 0; k < num_det; ++k) {
    if (result[0] == -1) {
      // Skip invalid detection.
      result += 7;
      continue;
    }
    vector<float> detection(result, result + 7);
    detections.push_back(detection);
    result += 7;
  }
  return detections;
}

/* Load the mean file in binaryproto format. */
void SingleShotDetector::SetMean(const string& mean_file, const string& mean_value) {
  cv::Scalar channel_mean;

  if (!mean_file.empty()) {
    if(mean_value.empty())
        std::cerr<<"Cannot specify mean_file and mean_value at the same time" <<endl;

    BlobProto blob_proto;
    ReadProtoFromBinaryFileOrDie(mean_file.c_str(), &blob_proto);

    /* Convert from BlobProto to Blob<float> */
    Blob<float> mean_blob;
    mean_blob.FromProto(blob_proto);
    if(mean_blob.channels() != num_channels_)
        std::cerr<< "Number of channels of mean file doesn't match input layer." <<endl;

    /* The format of the mean file is planar 32-bit float BGR or grayscale. */
    std::vector<cv::Mat> channels;
    float* data = mean_blob.mutable_cpu_data();
    for (int i = 0; i < num_channels_; ++i) {
      /* Extract an individual channel. */
      cv::Mat channel(mean_blob.height(), mean_blob.width(), CV_32FC1, data);
      channels.push_back(channel);
      data += mean_blob.height() * mean_blob.width();
    }

    /* Merge the separate channels into a single image. */
    cv::Mat mean;
    cv::merge(channels, mean);

    /* Compute the global mean pixel value and create a mean image
     * filled with this value. */
    channel_mean = cv::mean(mean);
    mean_ = cv::Mat(input_geometry_, mean.type(), channel_mean);

  }
  if (!mean_value.empty()) {

    if(!mean_file.empty())
        std::cerr<<  "Cannot specify mean_file and mean_value at the same time" <<endl;

    stringstream ss(mean_value);
    vector<float> values;
    string item;
    while (getline(ss, item, ',')) {
      float value = std::atof(item.c_str());
      values.push_back(value);
    }
    if(values.size() == (uint) 1 || values.size() == (uint) num_channels_)
        std::cerr<<"Specify either 1 mean_value or as many as channels: " << num_channels_ <<endl;

    std::vector<cv::Mat> channels;
    for (int i = 0; i < num_channels_; ++i) {
      /* Extract an individual channel. */
      cv::Mat channel(input_geometry_.height, input_geometry_.width, CV_32FC1,
          cv::Scalar(values[i]));
      channels.push_back(channel);
    }
    cv::merge(channels, mean_);
  }
}

/* Wrap the input layer of the network in separate cv::Mat objects
 * (one per channel). This way we save one memcpy operation and we
 * don't need to rely on cudaMemcpy2D. The last preprocessing
 * operation will write the separate channels directly to the input
 * layer. */
void SingleShotDetector::WrapInputLayer(std::vector<cv::Mat>* input_channels) {
  Blob<float>* input_layer = net_->input_blobs()[0];

  int width = input_layer->width();
  int height = input_layer->height();
  float* input_data = input_layer->mutable_cpu_data();
  for (int i = 0; i < input_layer->channels(); ++i) {
    cv::Mat channel(height, width, CV_32FC1, input_data);
    input_channels->push_back(channel);
    input_data += width * height;
  }
}

void SingleShotDetector::Preprocess(const cv::Mat& img, std::vector<cv::Mat>* input_channels){
  /* Convert the input image to the input image format of the network. */
  cv::Mat sample;
  if (img.channels() == 3 && num_channels_ == 1)
    cv::cvtColor(img, sample, cv::COLOR_BGR2GRAY);
  else if (img.channels() == 4 && num_channels_ == 1)
    cv::cvtColor(img, sample, cv::COLOR_BGRA2GRAY);
  else if (img.channels() == 4 && num_channels_ == 3)
    cv::cvtColor(img, sample, cv::COLOR_BGRA2BGR);
  else if (img.channels() == 1 && num_channels_ == 3)
    cv::cvtColor(img, sample, cv::COLOR_GRAY2BGR);
  else
    sample = img;

  cv::Mat sample_resized;
  if (sample.size() != input_geometry_)
    cv::resize(sample, sample_resized, input_geometry_);
  else
    sample_resized = sample;

  cv::Mat sample_float;
  if (num_channels_ == 3)
    sample_resized.convertTo(sample_float, CV_32FC3);
  else
    sample_resized.convertTo(sample_float, CV_32FC1);

  cv::Mat sample_normalized;
  cv::subtract(sample_float, mean_, sample_normalized);

  /* This operation will write the separate BGR planes directly to the
   * input layer of the network because it is wrapped by the cv::Mat
   * objects in input_channels. */
  cv::split(sample_normalized, *input_channels);

  if(reinterpret_cast<float*>(input_channels->at(0).data) != net_->input_blobs()[0]->cpu_data())
    std::cerr<<"Input channels are not wrapping the input layer of the network." <<endl;
}


void SingleShotDetector::forwardImage(cv::Mat &image){

  // Process image one by one.
    if(image.empty())
        std::cout<<"Unable to open the opencv image";


    std::vector<vector<float>> detections = Detect(image);  //SingleShotDetector.Detect(image);

    const float confidence_threshold = 0.1;

    /* Print the detection results. */
    int numOfDetectionResults = detections.size();

    bool bestClassFound = false;
    for (uint i = 0; i < numOfDetectionResults; ++i) {
        const vector<float>& d = detections[i];

        // Detection format: [image_id, label, score, xmin, ymin, xmax, ymax].
        const int classId = d[1];
        const float score = d[2];


        if (score >= confidence_threshold) {

            float xLeftBottom = d[3] * image.cols;
            float yLeftBottom = d[4] * image.rows;
            float xRightTop =   d[5] * image.cols;
            float yRightTop =   d[5] * image.rows;


            std::string className = ClassNames.at(classId) ;


            std::cout << "Detections[" <<i << "]="<<classId<< " confidence:" << d[2]
            <<   "  label=" << className << std::endl;

            //Draw Rectangle
            cv::Rect myRectangle = cv::Rect(xLeftBottom, yLeftBottom,xRightTop - xLeftBottom,yRightTop - yLeftBottom);
            cv::rectangle(image, myRectangle, cv::Scalar(0, 255, 0));


            //Display top class on the image+
            cv::putText(image, className.c_str() , cv::Point(xLeftBottom,yLeftBottom-5),
                cv::FONT_HERSHEY_DUPLEX, 1, cv::Scalar(100,240,100),2, CV_AA);


        }

    }

    ++frame_count;

}
#else
int main(int argc, char** argv) {
  std::cerr << "This example requires OpenCV; compile with USE_OPENCV." <<endl;
}
#endif  // USE_OPENCV

#else
int main(int argc, char** argv) {
  std::cerr << "This example requires CAFFE; compile with FLITR_USE_CAFFE." <<endl;
}
#endif //FLITR_USE_CAFFE
