#include "lib/AudioUtilities.h"
#include "lib/Constant.h"
#include "lib/Manager.h"

class ThreeImager : public juce::Component, private juce::Timer {
  public:
    ThreeImager(std::shared_ptr<Manager> data);
    ~ThreeImager() override;

    void paint(juce::Graphics &g) override;
    void resized() override;

    void timerCallback() override;

    void getDataForPaint();

  private:
    bool is_next_block_drawable = true;
    std::shared_ptr<Manager> manager;
    std::array<float[FFTConstants::FFT_LENGTH], 4> fft_data = {0.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThreeImager)
};