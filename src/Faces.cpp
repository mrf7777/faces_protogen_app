#include <protogen/IProtogenApp.hpp>
#include <protogen/IProportionProvider.hpp>
#include <protogen/Resolution.hpp>
#include <cmake_vars.h>

#include <cmath>

using namespace protogen;

class Faces : public protogen::IProtogenApp {
public:
    Faces()
        : m_deviceResolution(Resolution(0, 0)),
        m_mouthProvider(nullptr),
        m_active(false)
    {}

    std::string name() const override {
        return "Faces";
    }

    std::string id() const override {
        return PROTOGEN_APP_ID;
    }

    std::string description() const override {
        return "An app to display faces and move your mouth using a web interface and audio device.";
    }

    bool sanityCheck([[maybe_unused]] std::string& errorMessage) const override {
        return true;
    }

    void setActive(bool active) override {
        m_active = active;
    }

    void receiveResourcesDirectory([[maybe_unused]] const std::string& resourcesDirectory) override {
    }

    Endpoints serverEndpoints() const override {
        using httplib::Request, httplib::Response;
        return Endpoints{
            {
                Endpoint{HttpMethod::Get, "/home"},
                [](const Request&, Response& res){ res.set_content("This is the homepage.", "text/html"); }
            },
            {
                Endpoint{HttpMethod::Get, "/hello"},
                [](const Request&, Response& res){ res.set_content("Hello!", "text/plain"); }
            },
            {
                Endpoint{HttpMethod::Get, "/hello/website"},
                [](const Request&, Response& res){ res.set_content("Hello, website!", "text/plain"); }
            },
        };
    }

    std::string homePage() const override {
        return "/static/index.html";
    }

    std::string staticFilesDirectory() const override {
        return "/static";
    }

    std::string staticFilesPath() const override {
        return "/static";
    }

    std::string thumbnail() const override {
        return "/static/thumbnail.png";
    }

    void render(ICanvas& canvas) const override {
        if(m_mouthProvider) {
            // draw background
            canvas.fill(127, 127, 127);

            // draw based on mouth open proportion
            const double mouth_open_proportion = m_mouthProvider->proportion();
            const uint8_t value = std::floor(std::lerp(0.0, 255.0, mouth_open_proportion));
            canvas.drawPolygon({{64, 0}, {64 + 32, 0}, {64 + 32, 32}}, 0, value, value, true);
            canvas.drawEllipse(0, 0, 32, 32, 0, value, 0, true);
            canvas.drawLine(32, 0, 64, 32, value, 0, 0);
            canvas.drawLine(32, 32, 64 + 32, 0, 0, 0, value);
            // Imagine a circle at the right-most side of the canvas.
            // Draw a line from the center to the circle based on the mouth open proportion.
            const double angle = std::lerp(0.0, 2*M_PI, mouth_open_proportion);
            const double radius = 13;
            const double x = 64 + 32 + 16 + radius * std::cos(angle);
            const double y = 16 + radius * std::sin(angle);
            canvas.drawLine(64 + 32 + 16, 16, x, y, 0, value, 0);
            // draw outline of circle
            canvas.drawEllipse(64 + 32, 0, 32, 32, 0, 0, value, false);
        } else {
            canvas.fill(127, 0, 0);
        }
    }

    float framerate() const override {
        return 30;
    }

    void receiveDeviceResolution(const Resolution& resolution) override {
        m_deviceResolution = resolution;
    }

    std::vector<Resolution> supportedResolutions() const override {
        return {m_deviceResolution};
    }

    void setMouthProportionProvider(std::shared_ptr<IProportionProvider> provider) {
        m_mouthProvider = provider;
    }

private:
    Resolution m_deviceResolution;
    std::shared_ptr<IProportionProvider> m_mouthProvider;
    bool m_active;
};

// Interface to create and destroy you app.
// This is how your app is created and consumed as a library.
extern "C" IProtogenApp * create_app() {
    return new Faces();
}

extern "C" void destroy_app(IProtogenApp * app) {
    delete app;
}