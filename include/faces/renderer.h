#ifndef RENDERER_H
#define RENDERER_H

#include <tuple>
#include <cstdint>
#include <mutex>
#include <unordered_map>

#include <httplib.h>

#include <protogen/ICanvas.hpp>
#include <protogen/Proportion.hpp>
#include <faces/images.h>
#include <faces/protogen_head_state.h>

namespace protogen {

class EmotionDrawer final {
public:
	EmotionDrawer(const std::string& emotions_directory);
	void draw(ICanvas& canvas, ProtogenHeadState::Emotion emotion, Proportion eye_openness) const;
	void configWebServerToHostEmotionImages(httplib::Server& srv, const std::string& base_url_path);
	std::string emotionsSeparatedByNewline() const;
private:
	std::vector<ProtogenHeadState::Emotion> emotions() const;

	std::unordered_map<ProtogenHeadState::Emotion, ImageSpectrum> m_emotionImageSpectrums;
	std::string m_emotionsDirectory;
};

class ProtogenHeadFrameProvider final {
public:
	ProtogenHeadFrameProvider();
	void draw(ICanvas& canvas, ProtogenHeadState::Emotion emotion, Proportion mouth_openness, EmotionDrawer& emotion_drawer, ImageSpectrum& mouth_images, StaticImageDrawer& static_drawer, bool blank, Proportion eye_openness);
};

class Renderer final {
public:
    Renderer(EmotionDrawer emotion_drawer, const std::string& mouth_images_dir, const std::string& static_image_path);
    virtual void render(const ProtogenHeadState& data, ICanvas& canvas);
	const EmotionDrawer& emotionDrawer() const;
private:
	void viewProtogenHeadData(const ProtogenHeadState& data, ICanvas& canvas);

    EmotionDrawer m_emotionDrawer;
    StaticImageDrawer m_staticImageDrawer;
    ImageSpectrum m_headImages;
    ProtogenHeadFrameProvider m_protogenHeadFrameProvider;
    //mutable std::mutex m_mutex;
};

}	// namespace

#endif