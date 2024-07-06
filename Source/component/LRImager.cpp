#include "component/LRImager.h"

LRImager::LRImager(std::shared_ptr<Manager> data) : manager(data) { startTimerHz(60); }

LRImager::~LRImager() { stopTimer(); }

void LRImager::paint(juce::Graphics &g) {

};

void LRImager::resized() { repaint(); };

void LRImager::timerCallback() {
    if (is_next_block_drawable) {
        is_next_block_drawable = false;
        draw();
        repaint();
        is_next_block_drawable = true;
    }
};

void LRImager::draw() {

};