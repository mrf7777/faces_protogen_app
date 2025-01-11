#include <protogen/IProtogenApp.hpp>
#include <protogen/IProportionProvider.hpp>
#include <protogen/Resolution.hpp>
#include <protogen/StandardAttributeStore.hpp>
#include <faces/renderer.h>
#include <faces/protogen_head_state.h>
#include <cmake_vars.h>

#include <mutex>
#include <memory>
#include <cmath>
#include <atomic>
#include <thread>
#include <condition_variable>

using namespace protogen;

class Faces : public protogen::IProtogenApp {
public:
    Faces()
        : m_deviceResolution(Resolution(0, 0)),
        m_mouthProvider(nullptr),
        m_active(false),
        m_renderer(nullptr),
        m_state(),
        m_webServerThread(),
        m_webServerPort(-1),
        m_blinkingThread(),
        m_mouthThread(),
        m_resources(),
        m_attributes(std::shared_ptr<StandardAttributeStore>(new StandardAttributeStore()))
    {
        using namespace protogen::attributes;
        using Access = protogen::attributes::IAttributeStore::Access;
        m_attributes->adminSetAttribute(ATTRIBUTE_ID, PROTOGEN_APP_ID, Access::Read);
        m_attributes->adminSetAttribute(ATTRIBUTE_NAME, "Faces", Access::Read);
        m_attributes->adminSetAttribute(ATTRIBUTE_DESCRIPTION, "An app to display faces and move your mouth using a web interface and audio device.", Access::Read);
        m_attributes->adminSetAttribute(ATTRIBUTE_THUMBNAIL, "/static/thumbnail.png", Access::Read);
        m_attributes->adminSetAttribute(ATTRIBUTE_MAIN_PAGE, "/static/index.html", Access::Read);
        m_attributes->adminSetAttribute(ATTRIBUTE_HOME_PAGE, "https://github.com/mrf7777/faces_protogen_app", Access::Read);
    }

    bool sanityCheck([[maybe_unused]] std::string& errorMessage) const override {
        return true;
    }

    void initialize() override {
        std::cout << "Faces initializing with resources: " << m_resources << std::endl;
        const std::string emotions_directory = m_resources / std::filesystem::path("static/protogen_images/eyes");
        const std::string mouth_images_directory = m_resources / std::filesystem::path("static/protogen_images/mouth");
        const std::string static_image_path = m_resources / std::filesystem::path("static/protogen_images/static/nose.png");

        const EmotionDrawer emotion_drawer(emotions_directory);
        std::cout << "Faces initialized EmotionDrawer" << std::endl;

        m_renderer = std::unique_ptr<Renderer>(new Renderer(emotion_drawer, mouth_images_directory, static_image_path));
        std::cout << "Faces initialized Renderer" << std::endl;

        m_blinkingThread = std::thread(&Faces::blinkingThread, this);
        std::cout << "Faces initialized blinkingThread" << std::endl;
        m_mouthThread = std::thread(&Faces::mouthThread, this);
        std::cout << "Faces initialized mouthThread" << std::endl;

        std::cout << "Faces initialized" << std::endl;

        m_webServerThread = std::thread([this](){
            using httplib::Request, httplib::Response;
            auto server = httplib::Server();

            server.set_mount_point("/static", (m_resources / "static").string());

            server.Get("/emotion/all", [this](const Request&, Response& res){
                res.set_content(m_renderer->emotionDrawer().emotionsSeparatedByNewline(), "text/plain");
            });
            server.Get("/emotion", [this](const Request&, Response& res){
                const auto emotion = m_state.emotion();
                res.set_content(emotion, "text/html");
            });
            server.Put("/emotion", [this](const Request& req, Response&){
                m_state.setEmotion(req.body);
            });
            server.Get("/blank", [this](const Request&, Response& res){
                const auto blank = m_state.blank();
                const auto blank_string = blank ? "true" : "false";
                res.set_content(blank_string, "text/plain");
            });
            server.Put("/blank", [this](const Request& req, Response&){
                const auto blank = req.body == "true";
                m_state.setBlank(blank);
            });

            m_webServerPort = server.bind_to_any_port("0.0.0.0");
            server.listen_after_bind();
        });

    }

    void setActive(bool active) override {
        std::cout << "Faces setActive: " << active << std::endl;
        m_active = active;
    }

    static double normalize(double value, double min, double max) {
        return (value - min) / (max - min);
    }

    void blinkingThread() {        
        // Proportion animation over time
        //
        //     ^
        //     |
        //     (proportion)
        // 1.0 |**********       *********
        //     |          *     *
        //     |           *   *
        //     |            * *
        //     |             *
        // 0.0 |----------------------------(time)->
        //

        static constexpr double animation_action_start_time = 4.3;
        static constexpr double animation_action_end_time = 5.0;
        static constexpr double animation_action_mid_time = (animation_action_start_time + animation_action_end_time) / 2;
        static_assert(animation_action_start_time < animation_action_end_time);
        
        while(true) {
            // TODO: consider condition variable instead of busy waiting
            if(m_active) {
                double delay_seconds = 1.0 / framerate();

                const auto system_time = std::chrono::system_clock::now();
                const auto time_epoch = system_time.time_since_epoch();
                const double time_epoch_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_epoch).count() / 1000.0;
                const double animation_time = std::fmod(time_epoch_seconds, animation_action_end_time);

                Proportion eye_openness = Proportion::make(1.0).value();
                if(animation_time < animation_action_start_time) {
                    eye_openness = Proportion::make(1.0).value();
                } else if(animation_action_start_time <= animation_time && animation_time <= animation_action_mid_time) {
                    eye_openness = Proportion::make(std::lerp(1.0, 0.0, normalize(animation_time, animation_action_start_time, animation_action_mid_time))).value();
                } else if(animation_action_mid_time <= animation_time && animation_time <= animation_action_end_time) {
                    eye_openness = Proportion::make(std::lerp(0.0, 1.0, normalize(animation_time, animation_action_mid_time, animation_action_end_time))).value();
                } else {
                    eye_openness = Proportion::make(1.0).value();
                }
                m_state.setEyeOpenness(eye_openness);

                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int64_t>(delay_seconds * 1000)));
            }
        }
    }

    void mouthThread() {
        while(true) {
            // TODO: consider condition variable instead of busy waiting
            if(m_active) {
                m_state.setMouthOpenness(m_mouthProvider->proportion());
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(1000/framerate())));
            }
        }
    }

    void receiveResourcesDirectory([[maybe_unused]] const std::string& resourcesDirectory) override {
        m_resources = resourcesDirectory;
    }

    void receiveUserDataDirectory([[maybe_unused]] const std::string& userDataDirectory) override {
    }

    int webPort() const override {
        return m_webServerPort;
    }

    void render(ICanvas& canvas) const override {
        m_renderer->render(m_state, canvas);    
    }

    float framerate() const override {
        return 24.0;
    }

    void receiveDeviceResolution(const Resolution& resolution) override {
        m_deviceResolution = resolution;
    }

    std::vector<Resolution> supportedResolutions() const override {
        return {Resolution(128, 32)};
    }

    void setMouthProportionProvider(std::shared_ptr<IProportionProvider> provider) {
        m_mouthProvider = provider;
    }

    std::shared_ptr<attributes::IAttributeStore> getAttributeStore() override {
        return m_attributes;
    }

private:
    mutable std::mutex m_renderMutex;
    Resolution m_deviceResolution;
    std::shared_ptr<IProportionProvider> m_mouthProvider;
    std::atomic_bool m_active;
    std::unique_ptr<Renderer> m_renderer;
    mutable ProtogenHeadState m_state;
    std::thread m_webServerThread;
    int m_webServerPort;
    std::thread m_blinkingThread;
    std::thread m_mouthThread;
    std::filesystem::path m_resources;
    std::shared_ptr<StandardAttributeStore> m_attributes;
};

// Interface to create and destroy you app.
// This is how your app is created and consumed as a library.
extern "C" IProtogenApp * create_app() {
    return new Faces();
}

extern "C" void destroy_app(IProtogenApp * app) {
    delete app;
}