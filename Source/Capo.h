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
//  Capo.h
//  modularSynth
//
//  Created by Ryan Challinor on 1/5/14.
//
//

#ifndef __modularSynth__Capo__
#define __modularSynth__Capo__

#include <iostream>

#include "NoteEffectBase.h"
#include "IDrawableModule.h"
#include "Checkbox.h"
#include "Slider.h"

class Capo : public NoteEffectBase, public IDrawableModule, public IIntSliderListener
{
public:
   Capo();
   static IDrawableModule* Create() { return new Capo(); }


   void CreateUIControls() override;

   void SetEnabled(bool enabled) override { mEnabled = enabled; }

   //INoteReceiver
   void PlayNote(double time, int pitch, int velocity, int voiceIdx = -1, ModulationParameters modulation = ModulationParameters()) override;

   void CheckboxUpdated(Checkbox* checkbox) override;
   //IIntSliderListener
   void IntSliderUpdated(IntSlider* slider, int oldVal) override;

   virtual void LoadLayout(const ofxJSONElement& moduleInfo) override;
   virtual void SetUpFromSaveData() override;

private:
   struct NoteInfo
   {
      bool mOn{ false };
      int mVelocity{ 0 };
      int mVoiceIdx{ -1 };
      int mOutputPitch{ 0 };
   };

   //IDrawableModule
   void DrawModule() override;
   void GetModuleDimensions(float& width, float& height) override
   {
      width = mWidth;
      height = mHeight;
   }
   bool Enabled() const override { return mEnabled; }

   float mWidth{ 200 };
   float mHeight{ 20 };
   int mCapo{ 0 };
   IntSlider* mCapoSlider{ nullptr };
   std::array<NoteInfo, 128> mInputNotes;
   Checkbox* mRetriggerCheckbox{ nullptr };
   bool mRetrigger{ false };
};


#endif /* defined(__modularSynth__Capo__) */
