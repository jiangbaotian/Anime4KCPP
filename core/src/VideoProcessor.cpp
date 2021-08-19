#ifdef ENABLE_VIDEO

#include<exception>
#include<thread>

#include"ACCreator.hpp"
#include"VideoProcessor.hpp"

Anime4KCPP::VideoProcessor::VideoProcessor(const Parameters& parameters, const Processor::Type type, unsigned int threads)
    :threads(threads), param(parameters), type(type)
{
    totalFrameCount = fps = 0.0;
    width = height = 0;
}

Anime4KCPP::VideoProcessor::VideoProcessor(AC& config, unsigned int threads)
    :VideoProcessor(config.getParameters(), config.getProcessorType(), 
        threads ? threads : std::thread::hardware_concurrency()) {}

void Anime4KCPP::VideoProcessor::loadVideo(const std::string& srcFile)
{
    if (!videoIO.openReader(srcFile))
        throw ACException<ExceptionType::IO>("Failed to load file: file doesn't not exist or decoder isn't installed.");

    fps = videoIO.get(cv::CAP_PROP_FPS);
    totalFrameCount = videoIO.get(cv::CAP_PROP_FRAME_COUNT);
    height = static_cast<int>(std::round(param.zoomFactor * videoIO.get(cv::CAP_PROP_FRAME_HEIGHT)));
    width = static_cast<int>(std::round(param.zoomFactor * videoIO.get(cv::CAP_PROP_FRAME_WIDTH)));
}

void Anime4KCPP::VideoProcessor::setVideoSaveInfo(const std::string& dstFile, const Codec codec, const double fps)
{
    if (!videoIO.openWriter(dstFile, codec, cv::Size(width, height), fps))
        throw ACException<ExceptionType::IO>("Failed to initialize video writer.");
}

void Anime4KCPP::VideoProcessor::saveVideo()
{
    videoIO.release();
}

void Anime4KCPP::VideoProcessor::process()
{
    std::once_flag eptrFlag;
    std::exception_ptr eptr;

    videoIO.init(
        [&]()
        {
            Utils::Frame frame = videoIO.read();
            try
            { // Reduce memory usage
                auto ac = ACCreator::createUP(param, type);
                ac->loadImage(frame.first);
                ac->process();
                ac->saveImage(frame.first);
            }
            catch (...)
            {
                std::call_once(eptrFlag,
                    [&]()
                    {
                        videoIO.stopProcess();
                        eptr = std::current_exception();
                    });
                return;
            }
            videoIO.write(frame);
        }
    , threads).process();

    if (eptr)
        std::rethrow_exception(eptr);
}

void Anime4KCPP::VideoProcessor::processWithProgress(const std::function<void(double)>&& callBack)
{
    std::future<void> p = std::async(std::launch::async, &VideoProcessor::process, this);
    std::chrono::milliseconds timeout(1000);
    for (;;)
    {
        std::future_status status = p.wait_for(timeout);
        if (status == std::future_status::ready)
        {
            callBack(1.0);
            p.get();
            break;
        }
        double progress = videoIO.getProgress();
        callBack(progress);
    }
}

void Anime4KCPP::VideoProcessor::stopVideoProcess() noexcept
{
    if (videoIO.isPaused())
        videoIO.continueProcess();

    videoIO.stopProcess();
}

void Anime4KCPP::VideoProcessor::pauseVideoProcess()
{
    videoIO.pauseProcess();
}

void Anime4KCPP::VideoProcessor::continueVideoProcess()
{
    videoIO.continueProcess();
}

std::string Anime4KCPP::VideoProcessor::getInfo()
{
    std::ostringstream oss;
    oss << "----------------------------------------------" << '\n'
        << "Video information" << '\n'
        << "----------------------------------------------" << '\n'
        << "FPS: " << fps << '\n'
        << "Threads: " << threads << '\n'
        << "Total frames: " << totalFrameCount << '\n'
        << "----------------------------------------------" << '\n';
    return oss.str();
}

#endif // ENABLE_VIDEO
