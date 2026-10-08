#include "PluginEditor.h"
#include <cmath>
#include <vector>

namespace
{
const juce::Colour background { 0xff11191f }, ink { 0xffedf3f4 }, muted { 0xff93a8b0 }, grid { 0xff2c3b43 };
const juce::Colour mint { 0xff79d9bc }, coral { 0xffff8e80 }, amber { 0xffe1c68c };
juce::String hz(float f) { return f >= 1000 ? juce::String(f / 1000, f < 10000 ? 1 : 0) + "k" : juce::String(juce::roundToInt(f)); }
void text(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float size = 13, juce::Colour colour = muted,
          juce::Justification align = juce::Justification::centredLeft)
{
    const bool unicode=s.getNumBytesAsUTF8()>static_cast<size_t>(s.length());
    g.setColour(colour);
    // Match CJK glyph height to the Latin UI; this face has taller line metrics.
    g.setFont(unicode ? juce::FontOptions("Yu Gothic UI",size*1.35f,juce::Font::plain) : juce::FontOptions(size));
    g.drawText(s,r,align);
}
juce::Colour bandColour(const imagine::Band& b)
{
    if (!b.correlationValid) return muted;
    return b.correlation >= 0 ? amber.interpolatedWith(mint, b.correlation) : amber.interpolatedWith(coral, -b.correlation);
}
}
juce::String SoundImagineEditor::tr(const char* en, const char* ja) const
{
    return juce::String::fromUTF8(processor.language.load() == 1 ? ja : en);
}
SoundImagineEditor::SoundImagineEditor(SoundImagineProcessor& p) : AudioProcessorEditor(&p), processor(p), camera(p.readCamera())
{
    setOpaque(true); setResizable(true, true); setResizeLimits(720, 520, 1440, 1000);
    for (auto* button : { &threeD, &map, &freeze, &help, &axisX, &axisY, &axisZ, &home })
    {
        addAndMakeVisible(button);
        button->setColour(juce::TextButton::buttonColourId, grid);
        button->setColour(juce::TextButton::buttonOnColourId, mint.withAlpha(0.25f));
        button->setColour(juce::TextButton::textColourOffId, ink);
        button->setColour(juce::TextButton::textColourOnId, mint);
    }
    threeD.onClick = [this] { processor.view.store(0); repaint(); };
    map.onClick = [this] { processor.view.store(1); repaint(); };
    axisX.onClick = [this] { align(0); }; axisY.onClick = [this] { align(1); }; axisZ.onClick = [this] { align(2); };
    home.onClick = [this] { camera=imagine::Camera::home(); alignedAxis=-1; processor.saveCamera(camera); processor.view.store(0); repaint(); };
    freeze.onClick = [this] { frozen = !frozen; freeze.setToggleState(frozen, juce::dontSendNotification); repaint(); };
    help.onClick = [this] { showHelp = !showHelp; repaint(); };
    for (auto* combo : { &range, &languages })
    {
        addAndMakeVisible(combo);
        combo->setColour(juce::ComboBox::backgroundColourId, grid); combo->setColour(juce::ComboBox::textColourId, ink);
        combo->setColour(juce::ComboBox::outlineColourId, grid);
    }
    languages.addItem("English",1); languages.addItem(juce::String::fromUTF8("日本語"),2);
    languages.onChange = [this] { processor.language.store(languages.getSelectedId()-1); updateLanguage(); repaint(); };
    range.onChange = [this] { processor.floorDb.store(range.getSelectedId() == 1 ? -48 : range.getSelectedId() == 3 ? -90 : -72); repaint(); };
    data = processor.readSnapshot(); timerCallback(); setSize(900, 700); startTimerHz(30);
}
SoundImagineEditor::~SoundImagineEditor() { stopTimer(); }
void SoundImagineEditor::updateLanguage()
{
    lastLanguage=processor.language.load();
    map.setButtonText(tr("Map","平面")); freeze.setButtonText(tr("Freeze","保持")); home.setButtonText(tr("Home","初期視点"));
    range.clear(juce::dontSendNotification);
    for (int i=0; i<3; ++i) range.addItem(tr("Floor ","下限 ")+juce::String(i==0 ? -48 : i==1 ? -72 : -90)+" dB",i+1);
    range.setSelectedId(processor.floorDb.load()==-48 ? 1 : processor.floorDb.load()==-90 ? 3 : 2,juce::dontSendNotification);
    languages.setSelectedId(lastLanguage+1,juce::dontSendNotification);
    axisX.setTooltip(tr("Along X: Side versus RMS; frequency is hidden.","X軸方向：Sideとレベルの関係。周波数は重なります。"));
    axisY.setTooltip(tr("Along Y: frequency versus RMS; Side is hidden.","Y軸方向：周波数とレベル。Sideの差は重なります。"));
    axisZ.setTooltip(tr("Along Z: frequency versus Side; RMS is hidden.","Z軸方向：周波数とSide。レベルの差は重なります。"));
    threeD.setTooltip(tr("Drag to rotate. Wheel to zoom. Double-click to reset.","ドラッグで自由回転・ホイールで拡大縮小・ダブルクリックで初期視点。"));
}
void SoundImagineEditor::resized()
{
    const int x = getWidth() - 446;
    threeD.setBounds(x,24,40,28); map.setBounds(x+44,24,48,28); range.setBounds(x+98,24,114,28);
    freeze.setBounds(x+218,24,64,28); help.setBounds(x+288,24,28,28); languages.setBounds(x+322,24,98,28);
    axisX.setBounds(148,99,36,26); axisY.setBounds(190,99,36,26); axisZ.setBounds(232,99,36,26); home.setBounds(276,99,88,26);
    plot = { 64.f,180.f,static_cast<float>(getWidth()-128),static_cast<float>(getHeight()-350) };
}
void SoundImagineEditor::timerCallback()
{
    if (!frozen)
    {
        data=processor.readSnapshot();
        stale = data.frames == 0 || juce::Time::getMillisecondCounterHiRes()-data.capturedMs > 500;
    }
    if (lastLanguage!=processor.language.load()) updateLanguage();
    camera=processor.readCamera();
    alignedAxis=camera.alignedAxis();
    threeD.setToggleState(processor.view.load()==0,juce::dontSendNotification);
    map.setToggleState(processor.view.load()==1,juce::dontSendNotification);
    axisX.setToggleState(processor.view.load()==0 && alignedAxis==0,juce::dontSendNotification);
    axisY.setToggleState(processor.view.load()==0 && alignedAxis==1,juce::dontSendNotification);
    axisZ.setToggleState(processor.view.load()==0 && alignedAxis==2,juce::dontSendNotification);
    range.setSelectedId(processor.floorDb.load()==-48 ? 1 : processor.floorDb.load()==-90 ? 3 : 2,juce::dontSendNotification);
    repaint();
}
void SoundImagineEditor::align(int axis)
{
    camera=imagine::Camera::aligned(axis); alignedAxis=axis; processor.saveCamera(camera); processor.view.store(0); repaint();
}
imagine::Vec3 SoundImagineEditor::world(float f, float side, float level) const
{
    const float u=std::clamp(std::log(f/20)/std::log(data.bands.back().high/20),0.f,1.f);
    const float floor=static_cast<float>(processor.floorDb.load());
    const float v=std::clamp((level-floor)/-floor,0.f,1.f);
    return { (u-0.5f)*2, (side-0.5f)*1.4f, (v-0.5f)*1.4f };
}
juce::Point<float> SoundImagineEditor::screen(imagine::Vec3 v) const
{
    const auto a=camera.transform({2,0,0}), b=camera.transform({0,1.4f,0}), c=camera.transform({0,0,1.4f});
    const float extentX=std::abs(a.x)+std::abs(b.x)+std::abs(c.x), extentY=std::abs(a.y)+std::abs(b.y)+std::abs(c.y);
    const float scale=0.82f*camera.zoom*std::min(plot.getWidth()/std::max(0.01f,extentX),plot.getHeight()/std::max(0.01f,extentY));
    const auto q=camera.transform(v);
    return { plot.getCentreX()+q.x*scale,plot.getCentreY()-q.y*scale };
}
juce::Point<float> SoundImagineEditor::project(float f, float side, float level) const
{
    const auto v=world(f,side,level);
    if (processor.view.load()==1) return { plot.getX()+(v.x/2+0.5f)*plot.getWidth(),plot.getBottom()-(v.y/1.4f+0.5f)*plot.getHeight() };
    return screen(v);
}
juce::String SoundImagineEditor::viewGuide() const
{
    if (processor.view.load()==1 || alignedAxis==2)
        return tr("Z view: Which bands contain more Side? Check coral points for mono loss.","Z方向：どの帯域にSideが多い？ 赤い点はモノラル時の減衰も確認。 ");
    if (alignedAxis==0)
        return tr("X view: Is strong energy concentrated in Mid or Side? Frequencies overlap.","X方向：強い音はMid寄り？ Side寄り？ 周波数の違いは重なって見えます。");
    if (alignedAxis==1)
        return tr("Y view: Which bands are strong? This is the RMS spectrum; Side is hidden.","Y方向：どの帯域が強い？ RMSスペクトルとして見る。Sideの違いは重なります。");
    return tr("3D: Locate strong bands with high Side. Use X / Y / Z to separate the relationships.","3D：強くてSideの多い帯域を探す。X / Y / Zで関係を分けて確認。");
}
void SoundImagineEditor::drawPlot(juce::Graphics& g)
{
    const juce::Graphics::ScopedSaveState save(g);
    g.reduceClipRegion(0,170,getWidth(),getHeight()-306);
    const bool isMap=processor.view.load()==1;
    const float floor=static_cast<float>(processor.floorDb.load()), upper=data.bands.back().high;
    auto line=[&g](juce::Point<float> a,juce::Point<float> b,juce::Colour c,float width=1.f) { g.setColour(c); g.drawLine({a,b},width); };
    const auto origin=project(20,0,floor);
    // Grid planes establish the three data axes. Labels follow the camera.
    for (float f : {20.f,100.f,1000.f,10000.f,upper})
    {
        const auto a=project(f,0,floor), b=project(f,1,floor);
        line(a,b,grid);
        if (!isMap) line(b,project(f,1,0),grid.withAlpha(0.55f));
        if (isMap || alignedAxis!=0)
            text(g,hz(f),{a.x-22,a.y+8,44,18},11,muted,juce::Justification::centred);
    }
    for (float s : {0.f,0.5f,1.f})
    {
        const auto a=project(20,s,floor), b=project(upper,s,floor);
        line(a,b,grid);
        if (isMap)
            text(g,juce::String(juce::roundToInt(s*100))+"%",{a.x-44,a.y-9,38,18},11,muted,juce::Justification::centredRight);
        else if (alignedAxis!=1 && s>0)
            text(g,juce::String(juce::roundToInt(s*100))+"%",{a.x-22,a.y+8,44,18},11,muted,juce::Justification::centred);
    }
    if (!isMap)
    {
        for (float db : {floor,floor/2,0.f})
        {
            const auto a=project(20,0,db), b=project(upper,0,db);
            line(a,b,grid.withAlpha(0.65f));
            if (alignedAxis!=2) text(g,juce::String(juce::roundToInt(db)),{a.x-38,a.y-9,30,18},11,muted,juce::Justification::centredRight);
        }
        const std::array<juce::Point<float>,3> endpoints {project(upper,0,floor),project(20,1,floor),project(20,0,0)};
        const std::array<juce::String,3> titles {tr("X: Frequency / Hz","X: 周波数 / Hz"),tr("Y: Side / %","Y: Side / %"),tr("Z: RMS / dBFS","Z: レベル / dBFS")};
        for (int i=0;i<3;++i)
        {
            const auto endpoint=endpoints[static_cast<size_t>(i)];
            if (origin.getDistanceFrom(endpoint)<2) continue;
            g.setColour(i==0 ? mint : i==1 ? amber : coral);
            g.drawArrow({origin,endpoint},1.5f,7,5);
            auto delta=endpoint-origin; delta/=std::max(1.f,delta.getDistanceFromOrigin());
            const auto label=endpoint+delta*(std::abs(delta.x)>0.5f ? 80.f : 24.f);
            text(g,titles[static_cast<size_t>(i)],{label.x-69,label.y-9,138,18},11,ink,juce::Justification::centred);
        }
    }
    else
    {
        text(g,tr("X: Frequency / Hz","X: 周波数 / Hz"),{plot.getX(),plot.getBottom()+26,200,18},11);
        text(g,"Y: Side / %",{plot.getRight()-100,plot.getY()-24,120,18},11);
    }
    struct Dot { int index; juce::Point<float> p; float depth; };
    std::vector<Dot> dots;
    for (int i=0;i<imagine::bandCount;++i)
    {
        const auto& b=data.bands[static_cast<size_t>(i)];
        if (!b.active || b.levelDb<floor) continue;
        const float centre=std::sqrt(b.low*b.high);
        const auto p=project(centre,b.side,b.levelDb);
        const auto colour=bandColour(b);
        if (!isMap && alignedAxis!=2) line(project(centre,b.side,floor),p,colour.withAlpha(0.17f));
        dots.push_back({i,p,camera.transform(world(centre,b.side,b.levelDb)).z});
    }
    // Draw far points first. Each point is a measured band, not an interpolated source trajectory.
    std::sort(dots.begin(),dots.end(),[](const Dot& a,const Dot& b) { return a.depth<b.depth; });
    for (const auto& dot : dots)
    {
        const auto& b=data.bands[static_cast<size_t>(dot.index)];
        const float radius=isMap || alignedAxis==2 ? 3+6*std::clamp((b.levelDb-floor)/-floor,0.f,1.f) : 4.f;
        g.setColour(bandColour(b)); g.fillEllipse(dot.p.x-radius,dot.p.y-radius,2*radius,2*radius);
        if (dot.index==selected) { g.setColour(ink); g.drawEllipse(dot.p.x-radius-4,dot.p.y-radius-4,2*radius+8,2*radius+8,1.5f); }
    }
    // Orientation triad remains visible even when a data axis points at the viewer.
    if (!isMap)
    {
        const juce::Point<float> centre {plot.getRight()-30,plot.getBottom()-18};
        const std::array<imagine::Vec3,3> axes {{{1,0,0},{0,1,0},{0,0,1}}};
        for (int i=0;i<3;++i)
        {
            const auto v=camera.transform(axes[static_cast<size_t>(i)]);
            const auto p=centre+juce::Point<float>(v.x*28,-v.y*28);
            line(centre,p,i==0 ? mint : i==1 ? amber : coral,2);
            const std::array<juce::String,3> names {"X","Y","Z"};
            text(g,names[static_cast<size_t>(i)],{p.x-7,p.y-9,14,18},11,ink,juce::Justification::centred);
        }
    }
}
void SoundImagineEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);
    text(g,"sound imagine",{26,22,230,34},25,ink);
    text(g,tr("See the spectrum. Understand the stereo.","帯域の強さと、ステレオの関係を見る。"),{27,60,400,22},13);
    text(g,tr("Align to axis","軸方向に整列"),{27,99,116,26},13,ink);
    text(g,tr("Drag: rotate  /  Wheel: zoom","ドラッグ：回転 ／ ホイール：拡大縮小"),{382,99,static_cast<float>(getWidth()-409),26},12);
    text(g,viewGuide(),{27,138,static_cast<float>(getWidth()-54),26},13,ink);
    text(g,frozen ? tr("FROZEN","表示を保持中") : stale ? tr("NO RECENT AUDIO","入力更新なし") : tr("LIVE","測定中"),
        {static_cast<float>(getWidth()-218),65,190,20},11,frozen ? amber : stale ? muted : mint,juce::Justification::centredRight);
    drawPlot(g);
    const float y=static_cast<float>(getHeight()-114);
    g.setColour(grid); g.drawHorizontalLine(static_cast<int>(y-16),26,static_cast<float>(getWidth()-26));
    const auto& b=data.bands[static_cast<size_t>(selected)];
    text(g,hz(b.low)+" - "+hz(b.high)+" Hz",{27,y,210,26},20,ink);
    const float column=static_cast<float>(getWidth()-270)/4;
    const bool valid=b.active && data.frames>0;
    const std::array<juce::String,4> labels {tr("BAND RMS","帯域レベル"),tr("SIDE ENERGY","Sideの割合"),tr("L/R CORRELATION","左右の相関"),tr("L/R BALANCE","左右の偏り")};
    const std::array<juce::String,4> values {
        valid ? juce::String(b.levelDb,1)+" dBFS" : "--",
        valid ? juce::String(b.side*100,1)+"%" : "--",
        valid && b.correlationValid ? juce::String(b.correlation,2) : "--",
        !valid ? "--" : std::abs(b.balance)<0.01f ? tr("Centre","中央") : juce::String(std::abs(b.balance)*100,0)+"% "+(b.balance<0 ? tr("L","左") : tr("R","右"))};
    for (int i=0;i<4;++i)
    {
        const float x=250+static_cast<float>(i)*column;
        text(g,labels[static_cast<size_t>(i)],{x,y,column,18},11);
        text(g,values[static_cast<size_t>(i)],{x,y+22,column,24},17,i==2 ? bandColour(b) : ink);
    }
    text(g,tr("Hover over a point to inspect","点にマウスを合わせて詳細を見る"),{27,y+32,215,20},11);
    const auto monoLabel=tr("Mono sum: ","モノラル和：");
    const double monoDb=valid ? 10*std::log10(std::max(1.e-9f,1-b.side)) : 0;
    const juce::String mono=!valid ? monoLabel+"--" : b.side>0.9999f ? monoLabel+tr("cancellation","ほぼ消失") : monoLabel+juce::String(std::abs(monoDb)<0.05 ? 0 : monoDb,1)+" dB";
    text(g,mono,{27,y+55,230,20},11,amber);
    text(g,tr("CORRELATION","左右の相関"),{250,y+59,90,20},10);
    text(g,tr("-1 inverse","-1 逆相"),{346,y+59,95,20},11,coral);
    text(g,tr("0 unrelated","0 相関なし"),{446,y+59,110,20},11,amber);
    text(g,tr("+1 aligned","+1 同相"),{560,y+59,110,20},11,mint);
    text(g,juce::String(data.sampleRate/1000,1)+" kHz / "+juce::String(data.sampleRate/imagine::fftSize,1)+tr(" Hz bins"," Hz分解能"),
        {static_cast<float>(getWidth()-240),static_cast<float>(getHeight()-28),213,18},10,muted,juce::Justification::centredRight);
    if (processor.dropped.load()!=0)
        text(g,tr("Analysis overload: ","解析過負荷：")+juce::String(static_cast<juce::int64>(processor.dropped.load()))+tr(" samples skipped","サンプルをスキップ"),
            {27,static_cast<float>(getHeight()-28),400,18},11,coral);
    if (showHelp)
    {
        juce::Rectangle<float> box {35,134,static_cast<float>(getWidth()-70),static_cast<float>(getHeight()-158)};
        g.setColour(background.withAlpha(0.99f)); g.fillRoundedRectangle(box,10);
        g.setColour(grid); g.drawRoundedRectangle(box,10,1); box=box.reduced(20);
        text(g,tr("WHAT TO LOOK FOR","何を見るための点群か"),box.removeFromTop(30),20,ink);
        const std::array<juce::String,10> lines {
            tr("Each point is one frequency band. It is not the position of a sound source.","点は1つの周波数帯域。音源の空間位置を表すものではありません。"),
            tr("X = frequency, Y = Side energy, Z = band RMS. Colour = L/R correlation.","X＝周波数、Y＝Sideの割合、Z＝帯域レベル。色＝左右の相関。"),
            tr("Y view: Find strong / weak bands. Side differences overlap in this view.","Y方向：強い／弱い帯域を確認。Sideの違いはこの方向では重なります。"),
            tr("Z view: Find bands with high Side. Dot size conveys the hidden RMS level.","Z方向：Sideの多い帯域を確認。隠れるレベルは点の大きさで補います。"),
            tr("X view: Compare Side and strength. Different frequencies overlap; inspect a point.","X方向：Sideと強さの関係を見る。周波数は重なるので点の詳細も確認。"),
            tr("Example: strong bass + high Side + coral colour? Check bass in mono.","例：強い低音がSide側にあり赤い？ モノラルで低音の減衰を確認。"),
            tr("0% Side = identical L/R. 50% = equal Mid/Side. 100% = inverse L/R.","Side 0%＝左右同一、50%＝Mid/Side等パワー、100%＝左右逆相。"),
            tr("One-sided audio also has 50% Side. Correlation --; check L/R balance.","片側だけの音もSide 50%。相関は --。左右の偏りと合わせて判断。"),
            tr("Drag to rotate; wheel to zoom; double-click or Home to reset. Freeze holds data.","回転＝ドラッグ、拡大＝ホイール。初期視点で戻す。保持で固定。"),
            tr("High correlation is not a quality score. Low bands share FFT resolution.","相関が高いほど良い、という採点ではありません。低域はFFT分解能を共有。")};
        const float lineHeight=std::min(29.f,box.getHeight()/static_cast<float>(lines.size()));
        for (const auto& s : lines) text(g,s,box.removeFromTop(lineHeight),13);
    }
}
void SoundImagineEditor::selectAt(juce::Point<float> p)
{
    if (!plot.expanded(30).contains(p)) return;
    float nearest=24.f, bestDepth=-1.e9f;
    for (int i=0;i<imagine::bandCount;++i)
    {
        const auto& b=data.bands[static_cast<size_t>(i)];
        if (!b.active || b.levelDb<processor.floorDb.load()) continue;
        const float centre=std::sqrt(b.low*b.high);
        const float distance=p.getDistanceFrom(project(centre,b.side,b.levelDb));
        const float depth=camera.transform(world(centre,b.side,b.levelDb)).z;
        if (distance<nearest-0.5f || (std::abs(distance-nearest)<=0.5f && depth>bestDepth))
        { nearest=distance; bestDepth=depth; selected=i; }
    }
    repaint();
}
void SoundImagineEditor::mouseMove(const juce::MouseEvent& e) { if (!showHelp) selectAt(e.position); }
void SoundImagineEditor::mouseDown(const juce::MouseEvent& e)
{
    dragging=!showHelp && processor.view.load()==0 && plot.contains(e.position);
    dragStart=e.position; dragCamera=camera;
    if (!showHelp) selectAt(e.position);
}
void SoundImagineEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (!dragging) return;
    const float radius=std::min(plot.getWidth(),plot.getHeight())*0.5f;
    const auto from=(dragStart-plot.getCentre())/radius, to=(e.position-plot.getCentre())/radius;
    camera=dragCamera; camera.orbit(from.x,-from.y,to.x,-to.y);
    alignedAxis=-1; processor.saveCamera(camera); repaint();
}
void SoundImagineEditor::mouseWheelMove(const juce::MouseEvent& e,const juce::MouseWheelDetails& wheel)
{
    if (showHelp || processor.view.load()!=0 || !plot.contains(e.position)) return;
    camera.zoom=std::clamp(camera.zoom*std::exp(wheel.deltaY),0.5f,2.5f); processor.saveCamera(camera); repaint();
}
void SoundImagineEditor::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!showHelp && plot.contains(e.position) && processor.view.load()==0) home.onClick();
}
