/**
    bespoke synth, a software modular synthesizer
    Copyright (C) 2021 Ryan Challinor (contact: awwbees@gmail.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/
//
//  OscController.cpp
//  Bespoke
//
//  Created by Ryan Challinor on 5/1/14.
//
//

#include "OscController.h"
#include "SynthGlobals.h"

OscController::OscController(MidiDeviceListener* listener, std::string outAddress, int outPort, int inPort)
: mListener(listener)
, mConnected(false)
, mOutAddress(outAddress)
, mOutPort(outPort)
, mInPort(inPort)
, mOutputConnected(false)
{
   Connect();
}

OscController::~OscController()
{
   OSCReceiver::disconnect();
}

void OscController::Connect()
{
   try
   {
      bool connected = OSCReceiver::connect(mInPort);
      assert(connected);

      OSCReceiver::addListener(this);

      ConnectOutput();

      mConnected = true;
   }
   catch (std::exception e)
   {
   }
}

void OscController::ConnectOutput()
{
   if (mOutAddress != "" && mOutPort > 0)
   {
      mOutputConnected = mOscOut.connect(mOutAddress, mOutPort);
   }
}

bool OscController::SetInPort(int port)
{
   if (mInPort != port)
   {
      mInPort = port;
      OSCReceiver::disconnect();
      return OSCReceiver::connect(mInPort);
   }

   return false;
}

std::string OscController::GetControlTooltip(MidiMessageType type, int control)
{
   if (type == kMidiMessage_Control && control >= 0 && control < mOscMap.size())
      return mOscMap[control].mAddress;
   return "[unmapped]";
}

void OscController::SendValue(int page, int control, float value, bool forceNoteOn /*= false*/, int channel /*= -1*/)
{
   if (!mConnected)
      return;

   for (int i = 0; i < mOscMap.size(); ++i)
   {
      if (control == mOscMap[i].mControl) // && mOscMap[i].mLastChangedTime + 50 < gTime)
      {
         juce::OSCMessage msg(mOscMap[i].mAddress.c_str());

         if (mOscMap[i].mIsFloat)
         {
            mOscMap[i].mFloatValue = value;
            msg.addFloat32(mOscMap[i].mFloatValue);
         }
         else
         {
            mOscMap[i].mIntValue = value * 127;
            msg.addInt32(mOscMap[i].mIntValue);
         }

         if (mOutputConnected)
            mOscOut.send(msg);
      }
   }
}

void OscController::oscMessageReceived(const juce::OSCMessage& msg)
{
   std::string address = msg.getAddressPattern().toString().toStdString();

   if (address == "/jockey/sync")
   {
      std::string outputAddress = msg[0].getString().toStdString();
      std::vector<std::string> tokens = ofSplitString(outputAddress, ":");
      if (tokens.size() == 2)
      {
         mOutAddress = tokens[0];
         mOutPort = ofToInt(tokens[1]);
         ConnectOutput();
      }
      return;
   }

   if (msg.size() == 0 || (!msg[0].isFloat32() && !msg[0].isInt32()))
      return;

   // Handle note data and output these as notes instead of CC's.
   if (address.rfind("/note", 0) == 0 && msg.size() >= 2 && ((msg[0].isFloat32() && msg[1].isFloat32()) || (msg[0].isInt32() && msg[1].isFloat32() && msg[2].isFloat32())))
   {
      MidiNote note;
      note.mDeviceName = "osccontroller";
      note.mChannel = 1;
      int offset = 0;
      if (msg.size() >= 3 && msg[0].isInt32())
      {
         note.mChannel = msg[0].getInt32();
         offset = 1;
      }
      note.mPitch = msg[0 + offset].getFloat32();
      if (msg[1 + offset].getFloat32() < 1 / 127)
         note.mVelocity = 0;
      else
         note.mVelocity = msg[1 + offset].getFloat32() * 127;
      //ofLog() << "OSCNote: P: " << note.mPitch << " V: " << note.mVelocity << " ch:" << note.mChannel;
      if (mListener != nullptr)
         mListener->OnMidiNote(note);
      return;
   }

   for (int i = 0; i < msg.size(); ++i)
   {
      auto calculated_address = (i > 0) ? address + " " + std::to_string(i + 1) : address;
      int mapIndex = FindControl(calculated_address);

      bool isNew = false;
      if (mapIndex == -1) //create a new map entry
      {
         isNew = true;
         mapIndex = AddControl(calculated_address, msg[i].isFloat32());
      }

      MidiControl control;
      control.mControl = mOscMap[mapIndex].mControl;
      control.mDeviceName = "osccontroller";
      control.mChannel = 1;
      mOscMap[mapIndex].mLastChangedTime = gTime;
      if (mOscMap[mapIndex].mIsFloat)
      {
         assert(msg[i].isFloat32());
         mOscMap[mapIndex].mFloatValue = msg[i].getFloat32();
         control.mValue = mOscMap[mapIndex].mFloatValue * 127;
      }
      else
      {
         assert(msg[i].isInt32());
         mOscMap[mapIndex].mIntValue = msg[i].getInt32();
         control.mValue = mOscMap[mapIndex].mIntValue;
      }

      if (isNew)
      {
         MidiController* midiController = dynamic_cast<MidiController*>(mListener);
         if (midiController)
         {
            auto& layoutControl = midiController->GetLayoutControl(control.mControl, kMidiMessage_Control);
            layoutControl.mConnectionType = mOscMap[mapIndex].mIsFloat ? kControlType_Slider : kControlType_Direct;
         }
      }

      if (mListener != nullptr)
         mListener->OnMidiControl(control);
   }
}

int OscController::FindControl(std::string address)
{
   for (int i = 0; i < mOscMap.size(); ++i)
   {
      if (address == mOscMap[i].mAddress)
         return i;
   }

   return -1;
}

int OscController::AddControl(std::string address, bool isFloat)
{
   int existing = FindControl(address);
   if (existing != -1)
      return existing;

   OscMap entry;
   int mapIndex = (int)mOscMap.size();
   entry.mControl = mapIndex;
   entry.mAddress = address;
   entry.mIsFloat = isFloat;
   mOscMap.push_back(entry);

   return mapIndex;
}

namespace
{
   const int kSaveStateRev = 1;
}

void OscController::SaveState(FileStreamOut& out)
{
   out << kSaveStateRev;

   out << (int)mOscMap.size();
   for (size_t i = 0; i < mOscMap.size(); ++i)
   {
      out << mOscMap[i].mControl;
      out << mOscMap[i].mAddress;
      out << mOscMap[i].mIsFloat;
      out << mOscMap[i].mFloatValue;
      out << mOscMap[i].mIntValue;
      out << mOscMap[i].mLastChangedTime;
   }
}

void OscController::LoadState(FileStreamIn& in)
{
   int rev;
   in >> rev;
   LoadStateValidate(rev <= kSaveStateRev);

   int mapSize;
   in >> mapSize;
   mOscMap.resize(mapSize);
   for (size_t i = 0; i < mOscMap.size(); ++i)
   {
      in >> mOscMap[i].mControl;
      in >> mOscMap[i].mAddress;
      in >> mOscMap[i].mIsFloat;
      in >> mOscMap[i].mFloatValue;
      in >> mOscMap[i].mIntValue;
      in >> mOscMap[i].mLastChangedTime;
   }
}
