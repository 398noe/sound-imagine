#include "PluginEditor.h"
#include <cmath>
#include <vector>

namespace
{
const juce::Colour background { 0xff11191f }, ink { 0xffedf3f4 }, muted { 0xff93a8b0 }, grid { 0xff2c3b43 };
const juce::Colour mint { 0xff79d9bc }, coral { 0xffff8e80 }, amber { 0xffe1c68c };
juce::String hz(float f) { return f>=1000 ? juce::String(f/1000,f<10000 ? 1 : 0)+"k" : juce::String(juce::roundToInt(f)); }
void text(juce::Graphics& g,const juce::String& s,juce::Rectangle<float> r,float size=13,juce::Colour colour=muted,
          juce::Justification align=juce::Justification::centredLeft)
{
    const bool unicode=s.getNumBytesAsUTF8()>static_cast<size_t>(s.length());
    g.setColour(colour);
    g.setFont(unicode ? juce::FontOptions("Yu Gothic UI",size*1.35f,juce::Font::plain) : juce::FontOptions(size));
    g.drawText(s,r,align);
}
juce::Colour bandColour(const imagine::Band& b)
{
    if (!b.correlationValid) return muted;
    return b.correlation>=0 ? amber.interpolatedWith(mint,b.correlation) : amber.interpolatedWith(coral,-b.correlation);
}
void correlationMeter(juce::Graphics& g,juce::Rectangle<float> area,const imagine::Band& b)
{
    const float y=area.getCentreY();
    g.setColour(grid); g.fillRoundedRectangle(area.getX(),y-2,area.getWidth(),4,2);
    for (const float v : {0.f,0.5f,1.f}) {g.setColour(muted.withAlpha(0.6f)); g.drawVerticalLine(juce::roundToInt(area.getX()+v*area.getWidth()),y-4,y+4);}
    if (b.active && b.correlationValid)
    {
        const float x=area.getX()+(b.correlation+1)*0.5f*area.getWidth();
        g.setColour(bandColour(b)); g.fillRoundedRectangle(std::min(x,area.getCentreX()),y-2,std::max(1.f,std::abs(x-area.getCentreX())),4,2);
        g.fillEllipse(x-3,y-3,6,6);
    }
    text(g,"-1",{area.getX()-3,y+5,20,12},9); text(g,"0",{area.getCentreX()-8,y+5,16,12},9,muted,juce::Justification::centred);
    text(g,"+1",{area.getRight()-17,y+5,22,12},9,muted,juce::Justification::centredRight);
}
class BandTable final : public juce::Component,private juce::TableListBoxModel
{
public:
    BandTable()
    {
        addAndMakeVisible(table); table.setModel(this); table.setRowHeight(28);
        table.setColour(juce::ListBox::backgroundColourId,background); table.setColour(juce::ListBox::outlineColourId,grid);
        table.getHeader().setColour(juce::TableHeaderComponent::backgroundColourId,grid);
        table.getHeader().setColour(juce::TableHeaderComponent::textColourId,ink);
        table.getHeader().setColour(juce::TableHeaderComponent::outlineColourId,background);
        const int columnFlags=juce::TableHeaderComponent::defaultFlags & ~juce::TableHeaderComponent::sortable;
        table.getHeader().addColumn("Hz",1,150,50,-1,columnFlags); table.getHeader().addColumn("RMS / dBFS",2,110,50,-1,columnFlags);
        table.getHeader().addColumn("Mid / Side %",3,140,50,-1,columnFlags); table.getHeader().addColumn("Correlation",4,170,90,-1,columnFlags);
        table.getHeader().addColumn("Balance",5,110,50,-1,columnFlags); table.getHeader().addColumn("Mono / dB",6,110,50,-1,columnFlags);
        setSize(820,500);
    }
    void update(const imagine::Snapshot& next,int lang,int mode,bool held)
    {
        snapshot=next; language=lang; levelMode=mode; frozen=held;
        table.getHeader().setColumnName(1,lang==1 ? juce::String::fromUTF8("帯域 / Hz") : "Hz");
        table.getHeader().setColumnName(2,mode==0 ? "RMS / dBFS" : "PSD / dBFS/Hz");
        table.getHeader().setColumnName(4,lang==1 ? juce::String::fromUTF8("左右相関") : "Correlation");
        table.getHeader().setColumnName(5,lang==1 ? juce::String::fromUTF8("左右の偏り") : "Balance");
        table.getHeader().setColumnName(6,lang==1 ? juce::String::fromUTF8("モノラル和 / dB") : "Mono / dB");
        table.updateContent(); table.repaint(); repaint();
    }
    void resized() override {table.setBounds(getLocalBounds().withTrimmedBottom(26));}
    void paint(juce::Graphics& g) override
    {
        g.fillAll(background);
        text(g,juce::String(snapshot.numBands)+(language==1 ? juce::String::fromUTF8("帯域") : " bands")+" / "+juce::String(snapshot.fftPoints)+" FFT / "+
            juce::String(snapshot.fftPoints*1000.0/snapshot.sampleRate,1)+" ms / "+juce::String(snapshot.sampleRate/snapshot.fftPoints,2)+" Hz"+
            (frozen ? (language==1 ? juce::String::fromUTF8(" / 保持中") : " / Frozen") : ""),
            {10,static_cast<float>(getHeight()-24),static_cast<float>(getWidth()-20),22},12);
    }
private:
    int getNumRows() override {return snapshot.numBands;}
    void paintRowBackground(juce::Graphics& g,int row,int width,int height,bool selected) override
    {g.fillAll(selected ? grid : row%2==0 ? background : background.brighter(0.03f)); juce::ignoreUnused(width,height);}
    void paintCell(juce::Graphics& g,int row,int column,int width,int height,bool) override
    {
        if (row<0 || row>=snapshot.numBands) return;
        const auto& b=snapshot.bands[static_cast<size_t>(row)]; const bool valid=b.active && snapshot.frames>0;
        juce::String value;
        if (column==1) value=juce::String(b.low,1)+" - "+juce::String(b.high,1);
        if (column==2) value=valid ? juce::String(b.levelDb-(levelMode==1 ? 10*std::log10(b.high-b.low) : 0),1) : "--";
        if (column==3) value=valid ? juce::String((1-b.side)*100,1)+" / "+juce::String(b.side*100,1) : "--";
        if (column==4)
        {
            value=valid && b.correlationValid ? juce::String(b.correlation,2) : "--";
            correlationMeter(g,{48,1,static_cast<float>(width-58),16},b);
        }
        if (column==5) value=!valid ? "--" : std::abs(b.balance)<0.01f ? (language==1 ? juce::String::fromUTF8("中央") : "Centre") : juce::String(std::abs(b.balance)*100,1)+"% "+(b.balance<0 ? "L" : "R");
        if (column==6) value=!valid ? "--" : b.side>0.9999f ? "-inf" : juce::String(b.side<0.001f ? 0 : 10*std::log10(1-b.side),1);
        text(g,value,{6,0,static_cast<float>(column==4 ? 43 : width-12),static_cast<float>(height)},12,column==4 ? bandColour(b) : ink);
    }
    juce::TableListBox table;
    imagine::Snapshot snapshot;
    int language=1,levelMode=0;
    bool frozen=false;
};
class TableWindow final : public juce::DocumentWindow
{
public:
    explicit TableWindow(const juce::String& title) : DocumentWindow(title,background,closeButton,true)
    {setUsingNativeTitleBar(true); setResizable(true,false); setResizeLimits(700,280,1500,1000); setContentOwned(new BandTable,true);}
    void closeButtonPressed() override {setVisible(false);}
};
}
juce::String SoundImagineEditor::tr(const char* en,const char* ja) const
{
    return juce::String::fromUTF8(processor.language.load()==1 ? ja : en);
}
SoundImagineEditor::SoundImagineEditor(SoundImagineProcessor& p) : AudioProcessorEditor(&p),processor(p),camera(p.readCamera())
{
    setOpaque(true); setResizable(true,true); setResizeLimits(560,360,1440,1000);
    // Old Map state has the same semantics as a Z alignment.
    if (processor.view.exchange(0)==1) { camera=imagine::Camera::aligned(2); processor.saveCamera(camera); }
    for (auto* button : {&axisX,&axisY,&axisZ,&home,&freeze,&settings,&tableButton,&help})
    {
        addAndMakeVisible(button);
        button->setColour(juce::TextButton::buttonColourId,grid);
        button->setColour(juce::TextButton::buttonOnColourId,mint.withAlpha(0.25f));
        button->setColour(juce::TextButton::textColourOffId,ink);
        button->setColour(juce::TextButton::textColourOnId,mint);
    }
    axisX.onClick=[this] {align(0);}; axisY.onClick=[this] {align(1);}; axisZ.onClick=[this] {align(2);};
    home.onClick=[this] {camera=imagine::Camera::home(); alignedAxis=-1; processor.saveCamera(camera); repaint();};
    freeze.onClick=[this] {frozen=!frozen; freeze.setToggleState(frozen,juce::dontSendNotification); repaint();};
    settings.onClick=[this] {showSettings();}; help.onClick=[this] {showHelp=!showHelp; repaint();};
    tableButton.onClick=[this] {showTable();};
    data=processor.readSnapshot(); timerCallback(); setSize(760,540); startTimerHz(30);
}
SoundImagineEditor::~SoundImagineEditor() {stopTimer();}
void SoundImagineEditor::showTable()
{
    if (!tableWindow) {tableWindow=std::make_unique<TableWindow>(tr("All bands","全帯域の測定値")); tableWindow->centreAroundComponent(this,840,520);}
    auto* content=dynamic_cast<BandTable*>(tableWindow->getContentComponent());
    if (content) content->update(data,processor.language.load(),processor.levelMode.load(),frozen);
    tableWindow->setVisible(true); tableWindow->toFront(true);
}
void SoundImagineEditor::updateLanguage()
{
    lastLanguage=processor.language.load();
    axisX.setTooltip(tr("Side vs level (frequency hidden)","Mid / Sideと強さ（周波数は重なる）"));
    axisY.setTooltip(tr("Frequency vs level (Side hidden)","周波数と強さ（Sideは重なる）"));
    axisZ.setTooltip(tr("Frequency vs Side (level hidden)","周波数とMid / Side（強さは重なる）"));
    home.setTooltip(tr("Reset camera","視点をリセット")); freeze.setTooltip(tr("Freeze measurements","測定値の表示を保持"));
    settings.setTooltip(tr("FFT / averaging / level / language","FFT・平均化・レベル表示・言語")); help.setTooltip(tr("Reading the graph","グラフの読み方"));
    tableButton.setTooltip(tr("Show all bands in a separate window","全帯域の測定値を別ウィンドウで表示"));
}
void SoundImagineEditor::showSettings()
{
    juce::PopupMenu menu,floors,fft,averaging,levels,languages,bands;
    const int floor=processor.floorDb.load();
    for (int i=0;i<3;++i) {const int db=i==0 ? -48 : i==1 ? -72 : -90; floors.addItem(101+i,juce::String(db)+" dB",true,floor==db);}
    for (int order=11;order<=15;++order)
    {
        const int n=1<<order;
        fft.addItem(200+order,juce::String(n)+" / "+juce::String(n*1000.0/data.sampleRate,1)+" ms / "+juce::String(data.sampleRate/n,2)+" Hz",true,processor.fftOrderSetting.load()==order);
    }
    for (int i=0;i<3;++i) {const int ms=i==0 ? 50 : i==1 ? 250 : 1000; averaging.addItem(301+i,juce::String(ms)+" ms",true,processor.smoothingMs.load()==ms);}
    levels.addItem(401,"RMS / dBFS",true,processor.levelMode.load()==0);
    levels.addItem(402,"PSD / dBFS/Hz",true,processor.levelMode.load()==1);
    languages.addItem(501,"English",true,processor.language.load()==0); languages.addItem(502,juce::String::fromUTF8("日本語"),true,processor.language.load()==1);
    for (int i=0;i<3;++i) {const int n=32<<i; bands.addItem(601+i,juce::String(n),true,processor.displayBands.load()==n);}
    menu.addSubMenu(tr("Floor","表示下限"),floors); menu.addSubMenu(tr("FFT / window / bin spacing","FFT / 時間窓 / ビン間隔"),fft);
    menu.addSubMenu(tr("Power averaging","パワーの平均化"),averaging); menu.addSubMenu(tr("Level","レベル表示"),levels); menu.addSubMenu(tr("Language","言語"),languages);
    menu.addSubMenu(tr("Display bands","表示帯域数"),bands);
    const juce::Component::SafePointer<SoundImagineEditor> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&settings),[safe](int id)
    {
        if (!safe) return;
        auto& p=safe->processor;
        if (id>=101 && id<=103) p.floorDb.store(id==101 ? -48 : id==102 ? -72 : -90);
        if (id>=211 && id<=215) p.fftOrderSetting.store(id-200);
        if (id>=301 && id<=303) p.smoothingMs.store(id==301 ? 50 : id==302 ? 250 : 1000);
        if (id==401 || id==402) p.levelMode.store(id-401);
        if (id==501 || id==502) {p.language.store(id-501); safe->updateLanguage();}
        if (id>=601 && id<=603) p.displayBands.store(32<<(id-601));
        safe->repaint();
    });
}
void SoundImagineEditor::resized()
{
    const int x=getWidth()-300;
    int i=0;
    for (auto* button : {&axisX,&axisY,&axisZ,&home,&freeze,&settings,&tableButton,&help}) button->setBounds(x+i++*36,8,32,25);
    plot={56.f,48.f,static_cast<float>(getWidth()-112),static_cast<float>(getHeight()-150)};
}
void SoundImagineEditor::timerCallback()
{
    if (!frozen) {data=processor.readSnapshot(); stale=data.frames==0 || juce::Time::getMillisecondCounterHiRes()-data.capturedMs>500;}
    if (lastLanguage!=processor.language.load()) updateLanguage();
    camera=processor.readCamera(); alignedAxis=camera.alignedAxis();
    axisX.setToggleState(alignedAxis==0,juce::dontSendNotification); axisY.setToggleState(alignedAxis==1,juce::dontSendNotification); axisZ.setToggleState(alignedAxis==2,juce::dontSendNotification);
    if (tableWindow && tableWindow->isVisible())
    {
        tableWindow->setName(tr("All bands","全帯域の測定値"));
        if (auto* content=dynamic_cast<BandTable*>(tableWindow->getContentComponent())) content->update(data,processor.language.load(),processor.levelMode.load(),frozen);
    }
    repaint();
}
void SoundImagineEditor::align(int axis)
{
    camera=imagine::Camera::aligned(axis); alignedAxis=axis; processor.saveCamera(camera); repaint();
}
float SoundImagineEditor::level(const imagine::Band& b) const
{
    return b.levelDb-(processor.levelMode.load()==1 ? 10*std::log10(b.high-b.low) : 0);
}
imagine::Vec3 SoundImagineEditor::world(float f,float side,float db) const
{
    const float u=std::clamp(std::log(f/20)/std::log(data.bands[static_cast<size_t>(data.numBands-1)].high/20),0.f,1.f);
    const float floor=static_cast<float>(processor.floorDb.load());
    return {(u-0.5f)*2,(side-0.5f)*1.4f,(std::clamp((db-floor)/-floor,0.f,1.f)-0.5f)*1.4f};
}
juce::Point<float> SoundImagineEditor::screen(imagine::Vec3 v) const
{
    const auto a=camera.transform({2,0,0}),b=camera.transform({0,1.4f,0}),c=camera.transform({0,0,1.4f});
    const float extentX=std::abs(a.x)+std::abs(b.x)+std::abs(c.x),extentY=std::abs(a.y)+std::abs(b.y)+std::abs(c.y);
    const auto q=camera.transform(v);
    // Normalize each projected span to the viewport: the axes have different units,
    // so screen aspect is a display choice, not a physical distance metric.
    return {plot.getCentreX()+q.x*plot.getWidth()*camera.zoom/std::max(0.01f,extentX),
            plot.getCentreY()-q.y*plot.getHeight()*camera.zoom/std::max(0.01f,extentY)};
}
juce::Point<float> SoundImagineEditor::project(float f,float side,float db) const {return screen(world(f,side,db));}
void SoundImagineEditor::drawPlot(juce::Graphics& g)
{
    const juce::Graphics::ScopedSaveState save(g); g.reduceClipRegion(0,0,getWidth(),getHeight()-75);
    const float floor=static_cast<float>(processor.floorDb.load()),upper=data.bands[static_cast<size_t>(data.numBands-1)].high;
    auto line=[&g](juce::Point<float> a,juce::Point<float> b,juce::Colour c,float width=1.f) {g.setColour(c); g.drawLine({a,b},width);};
    const auto origin=project(20,0,floor);
    const std::array<float,10> ticks {20,50,100,200,500,1000,2000,5000,10000,20000};
    juce::Point<float> previous {-10000,-10000};
    for (float f : ticks)
    {
        if (f>upper) continue;
        const auto a=project(f,0,floor),b=project(f,1,floor);
        line(a,b,grid); if (alignedAxis!=2) line(b,project(f,1,0),grid.withAlpha(0.5f));
        if (alignedAxis!=0 && a.getDistanceFrom(previous)>32) {text(g,hz(f),{a.x-22,a.y+8,44,18},11,muted,juce::Justification::centred); previous=a;}
    }
    if (upper<20000) {const auto a=project(upper,0,floor); text(g,hz(upper),{a.x-22,a.y+8,44,18},11,muted,juce::Justification::centred);}
    for (float side : {0.f,0.25f,0.5f,0.75f,1.f})
    {
        const auto a=project(20,side,floor),b=project(upper,side,floor); line(a,b,grid);
        if (alignedAxis!=1)
        {
            const juce::String label=side==0 ? "Mid" : side==1 ? "Side" : juce::String(juce::roundToInt(side*100))+"%";
            if (label.isNotEmpty())
            {
                // X alignment makes Mid left and Side right; Z makes Mid bottom and Side top.
                if (alignedAxis==0) text(g,label,{a.x-44,a.y+9,88,18},11,ink,juce::Justification::centred);
                else text(g,label,{a.x-52,a.y-9,48,18},10,ink,juce::Justification::centredRight);
            }
        }
    }
    if (alignedAxis!=2)
    {
        const int step=plot.getHeight()>=260 ? 6 : 12;
        for (int db=static_cast<int>(floor);db<=0;db+=step)
        {
            const auto a=project(20,0,static_cast<float>(db)),b=project(upper,0,static_cast<float>(db));
            line(a,b,grid.withAlpha(db%12==0 ? 0.8f : 0.4f));
            if (db%12==0 || plot.getHeight()>300) text(g,juce::String(db),{a.x-38,a.y-9,30,18},11,muted,juce::Justification::centredRight);
        }
    }
    const std::array<juce::Point<float>,3> ends {project(upper,0,floor),project(20,1,floor),project(20,0,0)};
    for (int i=0;i<3;++i)
    {
        const auto endpoint=ends[static_cast<size_t>(i)]; if (origin.getDistanceFrom(endpoint)<2) continue;
        g.setColour(i==0 ? mint : i==1 ? amber : coral); g.drawArrow({origin,endpoint},1.2f,7,5);
    }
    const auto zEnd=ends[2];
    if (alignedAxis!=2) text(g,processor.levelMode.load()==0 ? "dBFS / RMS" : "dBFS/Hz / PSD",{zEnd.x-28,zEnd.y-26,125,18},11,ink);
    // Far points first; no connecting trajectory between independent band measurements.
    struct Dot {int index; juce::Point<float> p; float depth;}; std::vector<Dot> dots;
    for (int i=0;i<data.numBands;++i)
    {
        const auto& b=data.bands[static_cast<size_t>(i)]; const float db=level(b);
        if (!b.active || db<floor) continue;
        const float f=std::sqrt(b.low*b.high); const auto p=project(f,b.side,db);
        if (alignedAxis!=2) line(project(f,b.side,floor),p,bandColour(b).withAlpha(0.15f));
        dots.push_back({i,p,camera.transform(world(f,b.side,db)).z});
    }
    std::sort(dots.begin(),dots.end(),[](const Dot& a,const Dot& b) {return a.depth<b.depth;});
    for (const auto& dot : dots)
    {
        const auto& b=data.bands[static_cast<size_t>(dot.index)];
        const float radius=alignedAxis==2 ? 3+5*std::clamp((level(b)-floor)/-floor,0.f,1.f) : 3.5f;
        g.setColour(bandColour(b)); g.fillEllipse(dot.p.x-radius,dot.p.y-radius,2*radius,2*radius);
        if (dot.index==selected) {g.setColour(ink); g.drawEllipse(dot.p.x-radius-4,dot.p.y-radius-4,2*radius+8,2*radius+8,1.2f);}
    }
    const juce::Point<float> centre {static_cast<float>(getWidth()-26),static_cast<float>(getHeight()-107)};
    const std::array<imagine::Vec3,3> axes {{{1,0,0},{0,1,0},{0,0,1}}}; const std::array<juce::String,3> names {"X","Y","Z"};
    for (int i=0;i<3;++i)
    {
        const auto v=camera.transform(axes[static_cast<size_t>(i)]); const auto p=centre+juce::Point<float>(v.x*18,-v.y*18);
        line(centre,p,i==0 ? mint : i==1 ? amber : coral,1.5f); text(g,names[static_cast<size_t>(i)],{p.x-6,p.y-8,12,16},10,ink,juce::Justification::centred);
    }
}
void SoundImagineEditor::paint(juce::Graphics& g)
{
    g.fillAll(background); drawPlot(g);
    const float y=static_cast<float>(getHeight()-67),col=static_cast<float>(getWidth()-24)/5;
    g.setColour(grid); g.drawHorizontalLine(static_cast<int>(y-8),12,static_cast<float>(getWidth()-12));
    const auto& b=data.bands[static_cast<size_t>(std::min(selected,data.numBands-1))]; const bool valid=b.active && data.frames>0;
    const std::array<juce::String,5> labels {tr("Hz","帯域 / Hz"),processor.levelMode.load()==0 ? "RMS / dBFS" : "PSD / dBFS/Hz","Mid / Side %",tr("L/R correlation","左右相関"),tr("L/R balance","左右の偏り")};
    const std::array<juce::String,5> values {hz(b.low)+" - "+hz(b.high),valid ? juce::String(level(b),1) : "--",
        valid ? juce::String((1-b.side)*100,1)+" / "+juce::String(b.side*100,1) : "--",
        valid && b.correlationValid ? juce::String(b.correlation,2) : "--",
        !valid ? "--" : std::abs(b.balance)<0.01f ? tr("Centre","中央") : juce::String(std::abs(b.balance)*100,1)+"% "+(b.balance<0 ? tr("L","左") : tr("R","右"))};
    for (int i=0;i<5;++i)
    {
        const float x=12+i*col; text(g,labels[static_cast<size_t>(i)],{x,y,col,18},11);
        text(g,values[static_cast<size_t>(i)],{x,y+21,col,22},15,i==3 ? bandColour(b) : ink);
        if (i==3) correlationMeter(g,{x+45,y+20,col-55,16},b);
    }
    text(g,juce::String(data.fftPoints*1000.0/data.sampleRate,1)+" ms / "+juce::String(data.sampleRate/data.fftPoints,2)+" Hz / "+juce::String(data.sampleRate/1000,1)+" kHz",
        {12,static_cast<float>(getHeight()-20),300,16},10);
    const juce::String state=frozen ? tr("Frozen","保持中") : stale ? tr("No recent audio","入力更新なし") : "";
    text(g,state,{static_cast<float>(getWidth()-155),static_cast<float>(getHeight()-20),140,16},10,frozen ? amber : muted,juce::Justification::centredRight);
    if (processor.dropped.load()!=0) text(g,tr("Analysis skipped: ","解析スキップ：")+juce::String(static_cast<juce::int64>(processor.dropped.load())),{315,static_cast<float>(getHeight()-20),180,16},10,coral);
    if (showHelp)
    {
        juce::Rectangle<float> box {20,42,static_cast<float>(getWidth()-40),static_cast<float>(getHeight()-120)};
        g.setColour(background.withAlpha(0.99f)); g.fillRoundedRectangle(box,8); g.setColour(grid); g.drawRoundedRectangle(box,8,1); box=box.reduced(16);
        text(g,tr("Reading the graph","グラフの読み方"),box.removeFromTop(27),18,ink);
        const std::array<juce::String,9> lines {
            tr("X: frequency. Y: Mid / Side power ratio. Z: RMS or PSD. Each dot is a band.","X＝周波数、Y＝Mid / Sideのパワー比、Z＝RMSかPSD。点は帯域の測定値。"),
            tr("Y view: spectrum. Z view: stereo distribution. X view: Side versus strength.","Y方向＝スペクトル。Z方向＝ステレオの分布。X方向＝Sideと強さの関係。"),
            tr("Mid end: L=R. Side end: L=-R. The middle means equal Mid and Side power.","Mid端＝左右同一、Side端＝左右逆相。中央＝MidとSideのパワーが同じ。"),
            tr("The middle can mean unrelated stereo, one-sided audio, or 90-degree phase.","中央には無相関ステレオ、片側だけの音、90度の位相差などが含まれます。"),
            tr("Colour: green +1, sand 0, coral -1 correlation. Grey: undefined correlation.","色：緑＝相関+1、砂色＝0、赤＝-1、灰色＝相関を定義できない状態。"),
            tr("Coral + strong bass? Check mono loss. Side alone does not measure width.","強い低音が赤い？ モノラルでの減衰も確認。Sideだけで広がりは判定できません。"),
            tr("RMS: integrated band power. PSD: average power per Hz; flat for white noise.","RMS＝帯域内のパワー。PSD＝1 Hzあたりの平均パワー。白色ノイズで水平。"),
            tr("Longer FFT: finer bin spacing, slower response. Bin spacing = sample rate / N.","FFTを長くするとビンは細かく、反応は遅くなる。ビン間隔＝サンプルレート÷N。"),
            tr("LUFS is programme loudness with K weighting, not a per-band RMS label.","LUFSはK重み付けによる全体の音量評価。帯域RMSの単位変更ではありません。")};
        const float h=std::min(31.f,box.getHeight()/static_cast<float>(lines.size()));
        for (const auto& s : lines) text(g,s,box.removeFromTop(h),getWidth()<680 ? 11.f : 13.f);
    }
}
void SoundImagineEditor::selectAt(juce::Point<float> p)
{
    if (!plot.expanded(25).contains(p)) return;
    float nearest=24.f,bestDepth=-1.e9f;
    for (int i=0;i<data.numBands;++i)
    {
        const auto& b=data.bands[static_cast<size_t>(i)]; const float db=level(b); if (!b.active || db<processor.floorDb.load()) continue;
        const float f=std::sqrt(b.low*b.high),distance=p.getDistanceFrom(project(f,b.side,db)),depth=camera.transform(world(f,b.side,db)).z;
        if (distance<nearest-0.5f || (std::abs(distance-nearest)<=0.5f && depth>bestDepth)) {nearest=distance; bestDepth=depth; selected=i;}
    }
    repaint();
}
void SoundImagineEditor::mouseMove(const juce::MouseEvent& e) {if (!showHelp) selectAt(e.position);}
void SoundImagineEditor::mouseDown(const juce::MouseEvent& e)
{
    dragging=!showHelp && plot.contains(e.position); dragStart=e.position; dragCamera=camera; if (!showHelp) selectAt(e.position);
}
void SoundImagineEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (!dragging) return; const float radius=std::min(plot.getWidth(),plot.getHeight())*0.5f;
    const auto from=(dragStart-plot.getCentre())/radius,to=(e.position-plot.getCentre())/radius;
    camera=dragCamera; camera.orbit(from.x,-from.y,to.x,-to.y); alignedAxis=-1; processor.saveCamera(camera); repaint();
}
void SoundImagineEditor::mouseWheelMove(const juce::MouseEvent& e,const juce::MouseWheelDetails& wheel)
{
    if (showHelp || !plot.contains(e.position)) return; camera.zoom=std::clamp(camera.zoom*std::exp(wheel.deltaY),0.5f,2.5f); processor.saveCamera(camera); repaint();
}
void SoundImagineEditor::mouseDoubleClick(const juce::MouseEvent& e) {if (!showHelp && plot.contains(e.position)) home.onClick();}
