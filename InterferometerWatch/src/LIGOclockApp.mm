#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "StoredDataConnection.h"
#include "LiveDataConnection.h"
#include "GPSTime.hpp"
#include "CinderHelpers.hpp"
#include "DataParser.h"
#include "FilteredDataServer.h"
#include "ChannelStateManager.h"
#include "MomentRenderer.h"
#include "ChannelFilterFFT.h"
#include "TimelineRenderer.h"
#include "Slider.hpp"
#include "NavigationRenderer.h"
#include "ChannelDataIO.h"
#include "AppConfig.h"
#include "FontBook.hpp"

/*
 
 LIGOclockApp
 
 This is the "main" application class. The pipline, config, renderers etc. are all loaded up here.
 It also handles user input and passes those events off to their appropriate handlers.
 Cinder apps generally run as an update/draw loop. Rendering the UI using OpenGL at 60 fps, or 
 as close to that as it can get.
 
*/

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace ligo;
using namespace wdl;

typedef enum SelectionModes
{
    SelectionModeBundle = 0,
    SelectionModeCategory,
    SelectionModeFilter
    
} SelectionMode;

class LIGOclockApp : public AppNative
{
    FilteredDataPacketRef mDataPacket;
    
    time_t mPlayheadDataStartTime;
    time_t mPlayheadDataEndTime;
    double mCurrentDataTime;
    int mLastDataIndex;
    float mFFTFreqSelection;
    bool mIsUpdatingTimeline;
    
    shared_ptr<DataConnection> mDataConnection;
    
    SelectionMode mSelectionMode;
    
    MomentRenderer mMomentRenderer;
    TimelineRenderer mTimelineRenderer;
    
    float mScalarDataWindowTime;
    
    wdl::Slider mFFTRangeSlider;
    float mSliderMaxValue;
    
    NavigationRenderer mNavBar;
    
    NSWindow* newWindow;
    
public:
    
    LIGOclockApp() :
    mLastDataIndex(-1)
    ,mScalarDataWindowTime(0)
    ,mSelectionMode(SelectionModeBundle)
    ,mIsUpdatingTimeline(false)
    ,mFFTFreqSelection(800.0f)
    ,mSliderMaxValue(3000)
    {}
    
    void loadConfig();
    void prepareSettings(Settings *settings);
	void setup();
    void shutdown();
	
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void mouseUp( MouseEvent event );
    void mouseMove( MouseEvent event );
    void keyDown( KeyEvent event );
    
    void sortButtonWasClicked( Button *button );
    
    void selectNextFilter( int direction = 1 );
    void selectFilter(int filterIndex);
    void selectCategory(int catIndex);
    void selectBundle(int bundleIndex);
	
    void update();
    void updateDataForPlayheadTimestamp( time_t dataPlayheadTime );
    
	void draw();
    void drawPrimaryFFT();
    void drawTimeline();
    void drawMomentView();
    void drawNavigation();
    void drawFFTRangePicker();

};

#pragma mark - Setup

void LIGOclockApp::prepareSettings(Settings *settings)
{

#if DEBUG
    fs::path loadConfigPath = ci::app::AppBasic::getResourcePath("ligo_vis_config.debug.csv");
#else
    // NOTE: The config file needs to be in the same path as the app.
    // This should be automatically copied by XCode when the app is built.
    
    // NOTE: If this is running on Windows, this path might need to change.
    // It's making assumptions about where the resources are in relation to the
    // app itself.
    fs::path loadConfigPath = getResourcePath() / ".." / ".." / ".." / "ligo_vis_config.csv";
#endif
    loadConfigPath = fs::absolute(loadConfigPath);
    cout << "loadConfigPath: " << loadConfigPath << "\n";
    Config::Load( loadConfigPath );
    
    if ( Config::ShouldUseRetinaDisplay() )
    {
        settings->enableHighDensityDisplay();
    }
    if ( Config::ShouldUseFullScreen() )
    {
        settings->setFullScreen(true);
    }
    else
    {
        settings->setWindowSize(Config::ScreenSize());
    }
    settings->setResizable(false);
}

const static float kMarginBottomFFT = 18.f;
const static float kFFTReferenceLineHeight = 100.f;
const static float kFFTSliderHeight = 60.f;

void LIGOclockApp::setup()
{
    FontBook::loadFont("bold", "Blender-Bold.otf");
    FontBook::loadFont("book", "Blender-Book.otf");
    FontBook::loadFont("medium", "Blender-Medium.otf");
    FontBook::loadFont("heavy", "Blender-Heavy.otf");
    FontBook::loadFont("thin", "Blender-Thin.otf");
    
    Vec2f windowSize = getWindowSize();
    
#if DEBUG
    float circleY = windowSize.y * 0.5;
#else
    float circleY = windowSize.y * 0.55;
#endif
    
    mMomentRenderer.setInnerRadius(200); // nee 100
    mMomentRenderer.setCenterPosition(Vec2f(windowSize.x * 0.75,
                                            circleY));
    
    mTimelineRenderer.setOuterRadius(200);
    mTimelineRenderer.setCenterPosition(Vec2f(windowSize.x * 0.25,
                                              circleY));

    float sliderWidth = windowSize.x * 0.5;
    float sliderMargin = (windowSize.x - sliderWidth) * 0.5;
    mFFTRangeSlider = wdl::Slider(Rectf(sliderMargin,
                                        getWindowHeight() - kFFTSliderHeight - kMarginBottomFFT,
                                        sliderMargin + sliderWidth,
                                        getWindowHeight() - kMarginBottomFFT));
    
    mFFTRangeSlider.setValue(0.275f); // 900 hz
    mFFTRangeSlider.setShouldDrawTrack(false);
    mFFTRangeSlider.setSliderColor(ColorAf(1,1,1,1));
    mFFTRangeSlider.setSliderWidth(2 * Config::ScreenScale());
    
    long startTime;
    
    if ( Config::ShouldUseLiveData() )
    {
        console() << "Using LIVE data\n";
        mDataConnection = shared_ptr<LiveDataConnection>( new LiveDataConnection() );
        static_pointer_cast<LiveDataConnection>(mDataConnection)->openConnectionToNDServer(Config::ServerAddress(),
                                                                                           Config::ServerPort());
        startTime = mDataConnection->worldTimeToGPSDataTime( getTimeNow() );
    }
    else
    {
        console() << "Using CANNED data\n";
        mDataConnection = shared_ptr<StoredDataConnection>(new StoredDataConnection());
        StoredDataConnectionRef dataConnection = static_pointer_cast<StoredDataConnection>(mDataConnection);
        
        fs::path dataDirectory(Config::FlatFilesPath());
        dataConnection->openConnectionWithDataPath( dataDirectory );
        startTime = dataConnection->getDataStartTime();
    }
    
    mTimelineRenderer.setDataStartTime( startTime );
    mTimelineRenderer.setDuration(Config::TimelineDuration());

    mTimelineRenderer.setupGL();
    mMomentRenderer.setupGL();
    
    // This should be enough to spin-up the insances
    DataParser::getInstance();
    FilteredDataServer::getInstance();
    ChannelStateManager::getInstance();
    
    mNavBar.setup();
    mNavBar.getButtonSignal("sort")->connect( boost::bind( &LIGOclockApp::sortButtonWasClicked, this, _1 ) );
}

void LIGOclockApp::shutdown()
{
    mDataConnection->closeConnection();
    mNavBar.shutdown();
}

#pragma mark - Input

void LIGOclockApp::mouseDown( MouseEvent event )
{
    bool fftSliderContainsMouse = mFFTRangeSlider.contains(event.getPos());
    mFFTRangeSlider.setIsActive(fftSliderContainsMouse);
    if ( fftSliderContainsMouse )
    {
        mFFTRangeSlider.update(event.getPos());
    }
    else
    {
        mIsUpdatingTimeline = mTimelineRenderer.containsMousePosition(event.getPos());
    }
    
    mNavBar.mouseDown(event.getPos());
}

void LIGOclockApp::mouseDrag( MouseEvent event )
{
    if ( mFFTRangeSlider.getIsActive() )
    {
        mFFTRangeSlider.update(event.getPos());
    }
    else if ( mIsUpdatingTimeline )
    {
        mTimelineRenderer.updateClockWithMousePosition(event.getPos());
    }
    
    mNavBar.mouseDrag(event.getPos());
}

void LIGOclockApp::mouseUp( MouseEvent event )
{
    mIsUpdatingTimeline = false;
    if ( mFFTRangeSlider.getIsActive() )
    {
        mFFTRangeSlider.update(event.getPos());
    }
    mFFTRangeSlider.setIsActive(false);
    Vec2f mousePos = event.getPos();
    
    if ( mMomentRenderer.ringContainsMousePosition(mousePos) )
    {
        mMomentRenderer.mouseUp( mousePos );
    }

    mNavBar.mouseUp(event.getPos());
}

void LIGOclockApp::mouseMove( MouseEvent event )
{
    Vec2f mousePos = event.getPos();
    if ( mMomentRenderer.ringContainsMousePosition( event.getPos() ) )
    {
        mMomentRenderer.selectChannelAtMousePosition( mousePos );
    }
    else
    {
        mMomentRenderer.deselectChannel();
    }
    
    mNavBar.mouseMove(event.getPos());
}

void LIGOclockApp::keyDown( KeyEvent event )
{
    if ( event.getChar() == 'b' )
    {
        mSelectionMode = SelectionModeBundle;
    }
    else if ( event.getChar() == 'c' )
    {
        mSelectionMode = SelectionModeCategory;
    }
    else if ( event.getChar() == 'f' )
    {
        mSelectionMode = SelectionModeFilter;
    }
#if DEBUG
    else if ( event.getChar() == 'l' )
    {
        // Log out 1 frame
        mMomentRenderer.setShouldLog(true);
        mTimelineRenderer.setShouldLog(true);
    }
#endif
    else if ( event.getCode() == KeyEvent::KEY_LEFT )
    {
        // "Back"
        ChannelStateManager::getInstance().goBack();
    }
    else if ( event.getCode() == KeyEvent::KEY_UP )
    {
        mFFTFreqSelection = std::min((mFFTFreqSelection + 1.0f), 5000.0f);
        mFFTRangeSlider.setValue(mFFTFreqSelection / mSliderMaxValue);
    }
    else  if ( event.getCode() == KeyEvent::KEY_DOWN )
    {
        mFFTFreqSelection = std::max((mFFTFreqSelection - 1.0f), 0.0f);
        mFFTRangeSlider.setValue(mFFTFreqSelection / mSliderMaxValue);
    }
    else  if ( event.getChar() == ' ' )
    {
        // Play / pause
        mTimelineRenderer.setIsPaused(!mTimelineRenderer.getIsPaused());
    }
}

#pragma mark - Buttons

void LIGOclockApp::sortButtonWasClicked( Button *button )
{
    bool isSorted = !mMomentRenderer.getIsSorted();
    mMomentRenderer.setIsSorted(isSorted);
    mNavBar.setSortLabel( isSorted ? "Value" : "Bundle" );
}

#pragma mark - Selection

// Cycle through the filters
void LIGOclockApp::selectNextFilter( int direction )
{
    const std::vector<ChannelFilterRef> filters = DataParser::getInstance().getAllFilters();
    ChannelFilterRef &filter = ChannelStateManager::getInstance().getCurrentFilter();
    int numFilters = filters.size();
    int currentFilterIndex = find(filters.begin(), filters.end(), filter) - filters.begin();
    int filterIndex = (currentFilterIndex + direction) % numFilters;
    if ( filterIndex < 0 )
    {
        filterIndex = numFilters + filterIndex;
    }
    selectFilter( filterIndex );
}

void LIGOclockApp::selectFilter(int filterIndex)
{
    const std::vector<ChannelFilterRef> filters = DataParser::getInstance().getAllFilters();
    if ( filters.size() > filterIndex )
    {
        ChannelFilterRef selectedFilter = filters[filterIndex];
        ChannelStateManager::getInstance().selectFilter( selectedFilter );
    }
}

void LIGOclockApp::selectCategory(int catIndex)
{
    const std::deque<ChannelGroupingRef> navStack = ChannelStateManager::getInstance().getGroupingNavigation().getGroupingStack();
    std::map<std::string, CategoryRef> categoryOptions = navStack.back()->categories;
    if ( categoryOptions.size() > catIndex )
    {
        auto it = categoryOptions.begin();
        std::advance(it, catIndex);
        CategoryRef selectedCategory = it->second;
        ChannelStateManager::getInstance().selectCategory( selectedCategory );
    }
}

void LIGOclockApp::selectBundle(int bundleIndex)
{
    std::map<std::string, ChannelGroupingRef> bundles = ChannelStateManager::getInstance().getGroupingNavigation().getAllBundleGroupings();
    if ( bundles.size() > bundleIndex )
    {
        auto it = bundles.begin();
        std::advance(it, bundleIndex);
        ChannelGroupingRef selectedBundle = it->second;
        ChannelStateManager::getInstance().selectBundle( selectedBundle );
    }
}

#pragma mark - Update

void LIGOclockApp::update()
{
    mCurrentDataTime = mDataConnection->worldTimeToGPSDataTime( ligo::getTimeNow() );
    mTimelineRenderer.update( mCurrentDataTime );
    
    time_t clockPlayheadTime = mTimelineRenderer.getPlayheadTime();
    
    // Check to see if there's new data.
    int dataIndex = FilteredDataServer::getInstance().getDataIndexForTimestamp( clockPlayheadTime );
    if ( dataIndex != -1 && dataIndex != mLastDataIndex )
    {
        updateDataForPlayheadTimestamp( clockPlayheadTime );
        mLastDataIndex = dataIndex;
        // cout << "Moment view mLastDataIndex " << mLastDataIndex << "\n";
    }
    
    // Get the time
    // This needs to happen after updateDataForPlayheadTimestamp
    mScalarDataWindowTime = (clockPlayheadTime - double(mPlayheadDataStartTime)) /
                            (double(mPlayheadDataEndTime) - double(mPlayheadDataStartTime));
    
    mFFTFreqSelection = mSliderMaxValue * mFFTRangeSlider.getValue();
    
    // TODO: Log the horizontal axis
}

void LIGOclockApp::updateDataForPlayheadTimestamp( time_t dataPlayheadTime )
{
    mDataPacket = FilteredDataServer::getInstance().getDataForTimestamp( dataPlayheadTime );
    mPlayheadDataStartTime = mDataPacket->parsedData.dataGPSStartTime;
    mPlayheadDataEndTime = mDataPacket->parsedData.dataGPSEndTime;
}

#pragma mark - Draw

void LIGOclockApp::draw()
{
    gl::clear( Color( 0, 0, 0 ) );
    
    drawMomentView();
    
    drawTimeline();

    if ( ChannelStateManager::getInstance().getCurrentFilter()->getFilterKey() == kChannelFilterKeyFFT )
    {
        drawPrimaryFFT();
        drawFFTRangePicker();
    }
    
    drawNavigation();
}

void LIGOclockApp::drawFFTRangePicker()
{
    mFFTRangeSlider.render(true);
    Rectf fftRect = mFFTRangeSlider.getRect();
    gl::color(1,1,1);
    ligo::drawString(to_string((int)mFFTFreqSelection) + " Hz",
                     Vec2f(fftRect.x1 + (fftRect.getWidth() * mFFTRangeSlider.getValue()) - 15,
                           getWindowHeight() - 5),
                     0.2,
                     FontBook::getFont("bold"));
}

void LIGOclockApp::drawPrimaryFFT()
{
    if ( mDataPacket )
    {
        if ( mDataPacket->filterResultsByChannel.count( Config::TimelineFFTChannelName() ) > 0 )
        {
            FilterResultsRef darmFilterResults = mDataPacket->filterResultsByChannel[Config::TimelineFFTChannelName()][kChannelFilterKeyFFT];
            FilterFFTResultsRef darmFFTResults = static_pointer_cast<FilterFFTResults>(darmFilterResults);
            
            time_t dataStartTime = mDataPacket->parsedData.dataGPSStartTime;
            time_t dataEndTime = mDataPacket->parsedData.dataGPSEndTime;
            double dataDuration = dataEndTime - dataStartTime;
            
            double playheadTime = mTimelineRenderer.getPlayheadTime();
            float scalarNowTime = (playheadTime - dataStartTime) / dataDuration;
            int timeIndex = darmFFTResults->numSamples * scalarNowTime;
#if DEBUG
            if ( darmFFTResults->bandMagnitudes.size() <= timeIndex )
            {
                cout << "WARN: FFT band size (" << darmFFTResults->bandMagnitudes.size() <<") are <= timeIndex (" << timeIndex << ")\n";
                return;
            }
#endif
            vector<float> fftResults = darmFFTResults->bandMagnitudes[timeIndex];
            float frqHzBinInterval = darmFFTResults->frequencyInterval;            
            
            // Draw an FFT with this bitch
            int bandCount = darmFFTResults->numBands / 2.5;// / Limiting the Freq. spectrum (the end is boring)
            
            // Update the max value for the slider
            mSliderMaxValue = (bandCount * frqHzBinInterval);
            
            Rectf fftRect = mFFTRangeSlider.getRect();
            
            gl::color(ColorAf(0.5, 0.5, 0.5));
            
            for ( int x = 0; x < bandCount; ++ x )
            {
                float scalarMag = fftResults[x];

                float scalarX = (x / (float)bandCount);
                
                // TODO: Log the horizontal axis

                // Log vert
                float scalarY = log(1.0 + scalarMag);
                
                float lineX = fftRect.x1 + (scalarX * fftRect.getWidth());
                float lineY = getWindowHeight() - kMarginBottomFFT;

                gl::drawLine(Vec2f(lineX, lineY - (scalarY * kFFTReferenceLineHeight) ),
                             Vec2f(lineX, lineY));

            }
        }
    }
}

void LIGOclockApp::drawMomentView()
{
    map<string, double> filterUserInfo;
    filterUserInfo["freqHz"] = mFFTFreqSelection;
    filterUserInfo["scalarTime"] = mScalarDataWindowTime;    
    mMomentRenderer.draw( mDataPacket, mTimelineRenderer.getIsPaused(), &filterUserInfo );
}

void LIGOclockApp::drawNavigation()
{
    mNavBar.draw();    
}

void LIGOclockApp::drawTimeline()
{
    // Check if there's a primary signal and draw it.
    // NOTE: The primary signal may be a different packet than mDataPacket if
    // the clock is paused, but the signal keeps drawing.
    long dataIndex = FilteredDataServer::getInstance().getDataIndexForTimestamp( mCurrentDataTime );
    if ( dataIndex != -1 )
    {
        FilteredDataPacketRef nowDataPacket = FilteredDataServer::getInstance().getDataForTimestamp( mCurrentDataTime );
        if ( nowDataPacket )
        {
            if ( nowDataPacket->parsedData.channelData.count(Config::TimelineWaveformChannelName()) > 0 )
            {
                ChannelDataFrameRef timelineChannelData = nowDataPacket->parsedData.channelData[Config::TimelineWaveformChannelName()];
                mTimelineRenderer.drawPrimarySignal( timelineChannelData );
                mTimelineRenderer.drawHistory( mFFTFreqSelection );
            }
            else
            {
                cout << "ERROR: NO DARM SIGNAL\n";
            }
        }
//#if DEBUG
//        cout << nowDataPacket->parsedData.channelData.size() << " channels.\n";
//#endif
    }
#if DEBUG
//    else
//    {
//        cout << "Data index " << dataIndex << " for timestamp " << (long)mCurrentDataTime << "\n";
//    }
#endif
    // Clock-style
    mTimelineRenderer.draw();
    
#if DEBUG
    
    ligo::drawString("FPS:" + to_string((int)getAverageFps()), Vec2f(getWindowWidth() - 60, 20), 0.2f, FontBook::getFont("bold") );
    
#endif
}

CINDER_APP_NATIVE( LIGOclockApp, RendererGl(8) )
