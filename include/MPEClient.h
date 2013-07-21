//
//  MPEClient.h
//  Unknown Project
//
//  Copyright (c) 2013 William Lindmeier. All rights reserved.
//

#pragma once

#include <boost/asio.hpp>
#include "cinder/Rect.h"
#include "MPEMessageHandler.hpp"
#include "MPEProtocol.hpp"
#include "MPEProtocol2.hpp"
#include "TCPClient.h"

/*

 MPEClient:
 This class is the interface through which your App communicates with an MPE server.

 Create an instance of MPEClient in your Cinder app by passing in the path to a settings file.
 Each client will need it's own settings file. A template can be found at assets/settings.xml.

 The client keeps track of the current frame that should be rendered (see
 MPEMessageHandler::getCurrentRenderFrame) and informs the server when it's complete. Once
 all of the clients have rendered the frame the server will send out the next frame number.

 MPEClient uses callbacks for updating, drawing, and sending data to your App.

    • FrameUpdateCallback: This is the update callback, called whenever the server sends a new frame.
        App state changes should only happen in this callback, rather than in App::update() so that
        all of the clients stay in sync. This must be set.

    • FrameRenderCallback: This is the draw callback. The client will position the viewport before
        calling the callback and tells the server that the frame has been rendered after the callback.

    • StringDataCallback: This will be called when string data is received from any of the connected
        clients (including yourself).

    • IntegerDataCallback & BytesDataCallback: Similar to the above. These formats are not yet
        supported.

*/

namespace mpe
{
    typedef boost::function<void(bool isNewFrame)> FrameRenderCallback;
    typedef boost::function<void(long serverFrameNumber)> FrameUpdateCallback;
    typedef boost::function<void(const std::string & message, const int fromClientID)> StringDataCallback;

    class MPEClient : public MPEMessageHandler
    {

    public:

        // Constructors
        MPEClient(){};
        MPEClient(const std::string & settingsFilename, bool shouldResize = true);
        MPEClient(const std::string & settingsFilename, MPEProtocol * protocol, bool shouldResize = true);
        ~MPEClient(){};

        // Misc Accessors
        int                 getClientID();
        
        // Screen Dimensions
        ci::Rectf           getVisibleRect();
        void                setVisibleRect(const ci::Rectf & rect);
        ci::Vec2i           getMasterSize();

        // Callbacks
        void                setFrameUpdateCallback(const FrameUpdateCallback & callback);
        void                setDrawCallback(const FrameRenderCallback & callback);
        void                setStringDataCallback(const StringDataCallback & callback);

        // 3D Rendering
        bool                getIsRendering3D();
        void                setIsRendering3D(bool is3D);
        void                set3DFieldOfView(float fov);
        float               get3DFieldOfView();
        void                restore3DCamera();

        // Connection
        void                start();
        void                stop();
        bool                isConnected();

        // Loop
        virtual void        update();
        virtual void        draw();

        // Sending Data
        // Data sent to the server is broadcast to every client.
        // The sending App will receive its own data and should act on it when it's received,
        // rather than before it's sent, so all of the clients are in sync.
        void                sendStringData(const std::string & message); // née broadcast
        // Send data to specific client IDs
        void                sendStringData(const std::string & message,
                                           const std::vector<int> & clientIds);

    protected:
        
        virtual void        receivedStringMessage(const std::string & dataMessage,
                                                  const int fromClientID = -1);
        virtual void        receivedResetCommand();
        void                setCurrentRenderFrame(long frameNum);
        void                doneRendering();
        void                positionViewport();
        void                positionViewport3D();
        void                positionViewport2D();
        void                sendClientID();

        // A protocol to convert a given command into a transport string.
        std::shared_ptr<MPEProtocol> mProtocol;

        // Callbacks
        StringDataCallback  mStringDataCallback;
        FrameUpdateCallback mUpdateCallback;
        FrameRenderCallback mRenderCallback;

        bool                mIsRendering3D;
        long                mLastFrameConfirmed;

        // 3D Positioning
        float               mFieldOfView;
        float               mCameraZ;

        // Settings loaded from settings.xml
        int                 mPort;
        std::string         mHostname;
        bool                mIsStarted;
        ci::Rectf           mLocalViewportRect;
        ci::Vec2i           mMasterSize;
        int                 mClientID;
        bool                mIsDebug;

        // A connection to the server.
        TCPClient           *mTCPClient;

    private:

        void                tcpDidConnect();
        void                loadSettings(std::string settingsFilename, bool shouldResize);

    };
}
