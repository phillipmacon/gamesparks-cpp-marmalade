#include "s3e.h"
#include "IwDebug.h"
#include "IwUtilInitTerm.h"
#include "IwRuntime.h"

#include <stdio.h>
#include <GameSparks/GS.h>
#include <GameSparks/IGSPlatform.h>
#include <GameSparks/generated/GSRequests.h>
#include <GameSparks/generated/GSMessages.h>
#include <GameSparks/MarmaladePlatform.h>

using namespace GameSparks::Core;
using namespace GameSparks::Api::Messages;
using namespace GameSparks::Api::Responses;
using namespace GameSparks::Api::Requests;

void WithdrawChallengeRequest_Response(GameSparks::Core::GS& gsInstance, const WithdrawChallengeResponse& response)
{
    printf("withdraw challnage:\n");
    printf("%s\n", response.GetJSONString().c_str());
}

void GetChallengeRequest_Response(GameSparks::Core::GS& gsInstance, const GetChallengeResponse& response)
{
    printf("get challange data:\n");
    printf("%s\n", response.GetChallenge().GetJSONString().c_str());
    
    WithdrawChallengeRequest request(gsInstance);
    request.SetChallengeInstanceId(response.GetChallenge().GetChallengeId().GetValue());
    request.SetMessage("automated decline");
    request.Send(WithdrawChallengeRequest_Response);
}

void CreateChallengeRequest_Response(GameSparks::Core::GS& gsInstance, const GameSparks::Api::Responses::CreateChallengeResponse& response)
{
    printf("challange response: %s\n", response.GetJSONString().c_str());
    
    GetChallengeRequest request(gsInstance);
    request.SetChallengeInstanceId(response.GetBaseData().GetString("challengeInstanceId").GetValue());
    request.Send(GetChallengeRequest_Response);
}

// forward declaration
void GameSparksAvailable(GameSparks::Core::GS& gsInstance, bool available);

void RegistrationRequest_Response(GameSparks::Core::GS& gsInstance, const GameSparks::Api::Responses::RegistrationResponse& response)
{
    if (response.GetHasErrors())
    {
        printf("something went wrong during the authentication\n");
        printf("%s\n", response.GetErrors().GetValue().GetJSON().c_str());
    }
    else
    {
        printf("new user created\n");
        // instead of duplicating the code that send out an authentication request, we're just calling GameSparksAvailable here.
        GameSparksAvailable(gsInstance, true);
    }
}

void AuthenticationRequest_Response(GameSparks::Core::GS& gsInstance, const GameSparks::Api::Responses::AuthenticationResponse& response)
{
    // when we login successfully, we want to call a custom event
    if (response.GetHasErrors())
    {
        // In case you're running the sample the first time, the test user might not exist. So we just create it here
        static bool user_creation_tried = false;
        if (!user_creation_tried)
        {
            printf("Authentication failed, creating test user.\n");
            GameSparks::Api::Requests::RegistrationRequest request(gsInstance);
            request.SetDisplayName("SDK Sample Test User");
            request.SetUserName("abcdefgh");
            request.SetPassword("abcdefgh");

            // send the request
            request.Send(RegistrationRequest_Response);
        }
        else
        {
            printf("something went wrong during the authentication\n");
            printf("%s\n", response.GetErrors().GetValue().GetJSON().c_str());
        }
    }
    else
    {
        printf("you successfully authenticated to GameSparks with your credentials\n");
        printf("your displayname is %s\n", response.GetBaseData().GetString("displayName").GetValue().c_str());
        
        GSDateTime now = GSDateTime::Now();
        
        GameSparks::Api::Requests::CreateChallengeRequest challangeRequest(gsInstance);
        challangeRequest.SetAccessType("PUBLIC");
        challangeRequest.SetChallengeMessage("Ultimate Challange");
        challangeRequest.SetChallengeShortCode("ULTIMATECHALLANGE");
        challangeRequest.SetEndTime(now.AddHours(2).ToGMTime());
        challangeRequest.SetExpiryTime(now.AddMinutes(5).ToGMTime());
        challangeRequest.SetMaxPlayers(5);
        challangeRequest.SetMaxAttempts(5);
        challangeRequest.SetMinPlayers(2);
        challangeRequest.SetSilent(false);
        challangeRequest.SetStartTime(now.AddHours(1).ToGMTime());
        
        challangeRequest.Send(CreateChallengeRequest_Response);
    }
}

void GameSparksAvailable(GameSparks::Core::GS& gsInstance, bool available)
{
    printf("GameSparks is %s\n", (available ? "available" : "not available"));
    
    if (available)
    {
        // login immediately when gamesparks is available

        GameSparks::Api::Requests::AuthenticationRequest request(gsInstance);
        request.SetUserName("abcdefgh");
        request.SetPassword("abcdefgh");
        request.Send(AuthenticationRequest_Response);
    }
}


void OnAchievementEarnedMessage(GameSparks::Core::GS& gsInstance, const GameSparks::Api::Messages::AchievementEarnedMessage& message)
{
    printf("Achievement earned %s\n", message.GetAchievementName().GetValue().c_str());
}

int main()
{
    // this is here, so that marmalades memory management gets installed. This is needed, so that we can find out, if there are any leaks.
    IwUtilInit();
    
    IW_CALLSTACK("GameSparks Sample")
    {
        // create our GS-instance
        GameSparks::Core::GS GS;

        // here we create an instance of MarmaladePlatform. It stores the logs for us. Don't forget to insert your API Key and Secret below.
        MarmaladePlatform gsPlatform("exampleKey01", "exampleSecret0123456789012345678", true, true, 8);
        GS.Initialise(&gsPlatform);

        //Register our MessageHandler. SetMessageListener "auto-detects" the message type by the second argument to the MessageListener
        GS.SetMessageListener(OnAchievementEarnedMessage);
        
        // this event handler will login the user (see above)
        GS.GameSparksAvailable = GameSparksAvailable;
        
        int64 last_time = s3eTimerGetMs();
        while (!s3eDeviceCheckQuitRequest()) // this is the main loop
        {
            //Update the input systems
            s3eKeyboardUpdate();
            s3ePointerUpdate();
            
            s3eSurfaceClear(0, 0, 0);
            
            // Your rendering/app code goes here.
            s3eDebugPrint(0, 0, gsPlatform.getLog().c_str(), true);
            
            int64 now = s3eTimerGetMs();
            // update out GS-instance
            GS.Update((now-last_time) / 1000.0f);
            last_time = now;
            
            s3eSurfaceShow();
            
            // Sleep for 0ms to allow the OS to process events etc.
            s3eDeviceYield(0);
        }
        
        // shutdown out GS instance
        GS.ShutDown();
    }
    
    IwUtilTerminate();
}

