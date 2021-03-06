/*
* Copyright (c) 2013 Jonathan Perkin <jonathan@perkin.org.uk>
* Copyright (c) 2015 Elias Karakoulakis <elias.karakoulakis@gmail.com>
* 
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//=================================
#ifndef __OPENZWAVE_HPP_INCLUDED__
#define __OPENZWAVE_HPP_INCLUDED__

#include <iostream>
#include <list>
#include <queue>
#include <tr1/unordered_map>

#include <node.h>
#include <v8.h>
#include "nan.h"

#include "Manager.h"
#include "Driver.h"
#include "Node.h"
#include "Notification.h"
#include "Options.h"
#include "Value.h"

#define stringify( name ) # name

#ifdef WIN32
class mutex
{
public:
	mutex()              { InitializeCriticalSection(&_criticalSection); }
	~mutex()             { DeleteCriticalSection(&_criticalSection); }
	inline void lock()   { EnterCriticalSection(&_criticalSection); }
	inline void unlock() { LeaveCriticalSection(&_criticalSection); }

	class scoped_lock
	{
	public:
		inline explicit scoped_lock(mutex & sp) : _sl(sp) { _sl.lock(); }
		inline ~scoped_lock()                             { _sl.unlock(); }
	private:
		scoped_lock(scoped_lock const &);
		scoped_lock & operator=(scoped_lock const &);
		mutex& _sl;
	};

private:
	CRITICAL_SECTION _criticalSection;
};
#endif

#ifdef linux
#include <unistd.h>
#include <pthread.h>

class mutex
{
public:
	mutex()             { pthread_mutex_init(&_mutex, NULL); }
	~mutex()            { pthread_mutex_destroy(&_mutex); }
	inline void lock()  { pthread_mutex_lock(&_mutex); }
	inline void unlock(){ pthread_mutex_unlock(&_mutex); }

	class scoped_lock
	{
	public:
		inline explicit scoped_lock(mutex & sp) : _sl(sp)  { _sl.lock(); }
		inline ~scoped_lock()                              { _sl.unlock(); }
	private:
		scoped_lock(scoped_lock const &);
		scoped_lock & operator=(scoped_lock const &);
		mutex&  _sl;
	};

private:
	pthread_mutex_t _mutex;
};
#endif

using namespace v8;
using namespace node;

namespace OZW {
	
	struct OZW : public ObjectWrap {
		static NAN_METHOD(New);
		// openzwave-config.cc
		static NAN_METHOD(SetConfigParam);
		static NAN_METHOD(RequestConfigParam);
		static NAN_METHOD(RequestAllConfigParams);
		// openzwave-controller.cc
		static NAN_METHOD(HardReset);
		static NAN_METHOD(SoftReset);
		static NAN_METHOD(BeginControllerCommand);
		static NAN_METHOD(CancelControllerCommand);
		static NAN_METHOD(GetControllerNodeId);
		static NAN_METHOD(GetSUCNodeId);
		static NAN_METHOD(IsPrimaryController);
		static NAN_METHOD(IsStaticUpdateController);
		static NAN_METHOD(IsBridgeController);
		static NAN_METHOD(GetLibraryVersion);
		static NAN_METHOD(GetLibraryTypeName);
		static NAN_METHOD(GetSendQueueCount);
		// openzwave-driver.cc
		static NAN_METHOD(Connect);
		static NAN_METHOD(Disconnect);
		// openzwave-groups.cc
		static NAN_METHOD(GetNumGroups);
		static NAN_METHOD(GetAssociations);
		static NAN_METHOD(GetMaxAssociations);
		static NAN_METHOD(GetGroupLabel);
		static NAN_METHOD(AddAssociation);
		static NAN_METHOD(RemoveAssociation);
		// openzwave-network.cc
		static NAN_METHOD(TestNetworkNode);
		static NAN_METHOD(TestNetwork);
		static NAN_METHOD(HealNetworkNode);
		static NAN_METHOD(HealNetwork);
		// openzwave-nodes.cc
		static NAN_METHOD(GetNodeNeighbors);
		static NAN_METHOD(RefreshNodeInfo);
		static NAN_METHOD(SwitchAllOn);
		static NAN_METHOD(SwitchAllOff);
		// openzwave-values.cc
		static NAN_METHOD(SetValue);
		static NAN_METHOD(SetLocation);
		static NAN_METHOD(SetName);
		// openzwave-polling.cc
		static NAN_METHOD(GetPollInterval);
		static NAN_METHOD(SetPollInterval);
		static NAN_METHOD(EnablePoll);
		static NAN_METHOD(DisablePoll);
		static NAN_METHOD(IsPolled);
		static NAN_METHOD(SetPollIntensity);
		static NAN_METHOD(GetPollIntensity);
		// openzwave-scenes.cc
		static NAN_METHOD(CreateScene);
		static NAN_METHOD(RemoveScene);
		static NAN_METHOD(GetScenes);
		static NAN_METHOD(AddSceneValue);
		static NAN_METHOD(RemoveSceneValue);
		static NAN_METHOD(SceneGetValues);
		static NAN_METHOD(ActivateScene);
		//
		
		//
	};

	// callback struct to copy data from the OZW thread to the v8 event loop: 
	typedef struct {
		uint32_t type;
		uint32_t homeid;
		uint8_t nodeid;
		uint8_t groupidx;
		uint8_t event;
		uint8_t buttonid;
		uint8_t sceneid;
		uint8_t notification;
		std::list<OpenZWave::ValueID> values;
		OpenZWave::Driver::ControllerState state;
		OpenZWave::Driver::ControllerError err;
	} NotifInfo;
	
	typedef struct {
		uint32_t homeid;
		uint8_t nodeid;
		bool polled;
		std::list<OpenZWave::ValueID> values;
	} NodeInfo;

	typedef struct {
		uint32_t sceneid;
		std::string label;
		std::list<OpenZWave::ValueID> values;
	} SceneInfo;
	
	/*  
	 */
	extern uv_async_t 		async;

	/*
	* Message passing queue between OpenZWave callback and v8 async handler.
	*/
	extern mutex zqueue_mutex;
	extern std::queue<NotifInfo *> zqueue;

	/*
	* Node state.
	*/
	extern mutex znodes_mutex;
	extern std::list<NodeInfo *> znodes;

	extern mutex zscenes_mutex;
	extern std::list<SceneInfo *> zscenes;
	
	// our ZWave Home ID
	extern uint32_t homeid;

	Local<Object> zwaveValue2v8Value(OpenZWave::ValueID value);
	Local<Object> zwaveSceneValue2v8Value(uint8 sceneId, OpenZWave::ValueID value);
	
	NodeInfo *get_node_info(uint8_t nodeid);
	SceneInfo *get_scene_info(uint8_t sceneid);
	
	// OpenZWave callbacks
	void ozw_watcher_callback(
		OpenZWave::Notification const *cb, 
		void *ctx);
	void ozw_ctrlcmd_callback(
		OpenZWave::Driver::ControllerState _state, 
		OpenZWave::Driver::ControllerError _err, 
		void *ctx);
		
	// v8 asynchronous callback handler
	void async_cb_handler(uv_async_t *handle);
	void async_cb_handler(uv_async_t *handle, int status);
	
	//extern Handle<Object>	context_obj;
	extern NanCallback *emit_cb;
	//
	
	// map of controller command names to enum values
	typedef ::std::tr1::unordered_map <std::string, OpenZWave::Driver::ControllerCommand> CommandMap;
	extern CommandMap* ctrlCmdNames;
	
}

#endif // __OPENZWAVE_HPP_INCLUDED__
