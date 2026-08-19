# log #1: init
So the new Daisy Seed 3 came out! And I want to take the opportunity to make a few cool digital signal processing projects!
![image](https://cdn.hackclub.com/019fa43e-fa46-7bb9-933f-ff94e86c11d5/image.png) 
![image](https://cdn.hackclub.com/019fa43f-32b9-7a78-b12d-591aa9a06b0a/image.png)
I'm imagining a little portable synth that can record samples or receive line in, midi in, and then output line out, with an included headphone jack and small speaker. I'm sure the design will change over time but the new daisy seed 3 included so many new circuit diagrams, and it's really helpful for this. 

As I learn more about digital signal processing, I want for this to be able to function like a chompi, or a po-33, or a hichord.


# log #2: daisy documentation
Figuring out how the daisy seed 3 handles input and output... 
![image](https://cdn.hackclub.com/019fa967-1c68-79b7-bcb8-7c5285a68385/image.png)


# log #3: io ports
![image](https://cdn.hackclub.com/019fae99-44d2-7a57-86ec-1224aacefd0c/image.png)
decided on io ports. I looked at a lot of samplers and thought about what i should include. so there's MIDI in out, Audio in out, and instrument level in and headphones


# log #4: final-ish design
This is probably the final design on this sampler. I'd like to have joystick for real-time effects and I got rid of the midi-in and midi-out because that can be done over usb-c
![image](https://cdn.hackclub.com/019fb7e4-e71e-7dd5-a027-8a5b76d5b90a/image.png)
![image](https://cdn.hackclub.com/019fb7e5-4685-78e4-9979-3063c0800027/image.png)
design done ! time to do the circuits...


# log #5: bom assemble
![image](https://cdn.hackclub.com/019fbb73-6f5a-77d3-b110-9352e7424b97/image.png)
spent some time assembling the bom! might have to use amazon for now because I don't know if I have so much time to wait for shipping...


# log #6: rearranging some more & schematic
![image](https://cdn.hackclub.com/019fc58d-059b-7b0d-b82f-2446f6fc9f35/image.png)
So I want something that sort of functions like this for drum beats, and like the chompi for pitch samples.

That’s sort of part of how I wanted to design the buttons and the io.


# log #6.5: op-amps...
![image](https://cdn.hackclub.com/019fcae8-f37e-7b29-84f9-8f0a8d2d5b44/image.png)
Current bane of my existence. I need to learn about them. They're so important for audio processing and also just electronics in general. 

Time to lock in.


# log #7: gpio expanders for the inputs !
![image](https://cdn.hackclub.com/019fd530-cf00-7a53-9a83-97594a91da10/image.png)
added a few gpio i2c expanders. I think most of this is just drawing out the available pins and mapping out available slots. looked through the datasheets!

# log #8: wiring gpio expanders
![image](https://cdn.hackclub.com/019ff4ab-97f4-7ff8-a256-222b7f05c2ba/image.png)
![image](https://cdn.hackclub.com/019ff4ac-1b6e-7c2c-ae84-217f3e5e5989/image.png)
Wired up the GPIO expanders! Learned that I can just assign nets to the PCB and then apply changes to the schematic from the PCB, so this was so, so useful; no more cross referencing the schematic and switching back and forth.

# log #9: wiring op-amps
![image](https://cdn.hackclub.com/019ffb9b-add1-7d97-87f3-cc65296aa526/image.png)
Honestly a lot more complicated than I expected, I still don't quite understand all of it, but it's a work in progress!


# log #10: picking a microphone...
So, I've decided that I'm not adding a battery to this; I want to keep the form factor slim, plus, I want to keep all the components soldered to the top/through-hole, and having the battery sort of defeats that. 

Time to pick a mic!
![image](https://cdn.hackclub.com/019fff20-7c75-7217-92d3-30d97a1adbc0/image.png)
Decided to just go with a digital MEMS style mic, it's fairly low power, so that should be fine. 
![image](https://cdn.hackclub.com/019fff18-f8c2-7baa-ae42-cb3440f8c386/image.png)
Finished the schematic!!
- Made a few additional changes: 
- Changed the dual pot for a smaller thumbwheel-esque thing. 
- Made everything powered directly from the DaisySeed 3V3. 

Time to PCB...
![image](https://cdn.hackclub.com/019fff21-b97e-7ffc-9653-ffbfb109f231/image.png)


# log #11: pcb time...
Finally finished wiring....
![image](https://cdn.hackclub.com/01a002b7-67ff-79e0-8ec7-1e15ff3f0b5c/image.png)
![image](https://cdn.hackclub.com/01a002b7-f66e-772e-8d2c-9c2f57965a94/image.png)
A surprising amount of space leftover on the headphone side! Could have made this a bit more compact, but I'd rather leave a bit of space for troubleshooting. 

God, this took too long.
![image](https://cdn.hackclub.com/01a002ba-941a-77c4-aa0a-6bb21ceddc02/image.png)
![image](https://cdn.hackclub.com/01a002bb-3bba-76d4-8859-6d7433e5ead5/image.png)
Entirely top mounted! I'd like to have a more open design for the case, or even just a flat sheet of acrylic to the bottom. I do like the bare pcb look, but I do want to add some silkscreen art.


# log #12: logo design + moving around the pcb
A lot of browsing pinterest, then deciding that it had absolutely none of what I wanted and I should just make a logo myself (canva). I do want to make a poster for this later, so that should be fun!
![image](https://cdn.hackclub.com/01a00b73-f66c-7434-a045-79def1d61aa6/image.png)
I think it's pretty neat! The V and the P just so happen to be exactly the right shape for the two things I need (fang and music note). It should convert nicely to an SVG. 
![image](https://cdn.hackclub.com/01a00b75-8a85-7785-81f7-957f1750d71f/image.png)
All the extra space in the PCB did come in handy! I cleared out a bigger space to put the logo.


# log #13: finishing touches
![image](https://cdn.hackclub.com/01a00fc6-06c9-7e71-969d-78797d6cf6e9/image.png)
Added some silkscreen art! And I realized that you can specify which color your solder mask is in KiCad, so that's pretty cool.

# log #14: fixing the mic
So I went through quite a bit with research and changing around the mic. Apparently JLCPCB does not handle MEMS mic assembly very well, so I'm going to exclude it for assembly. And because it's not in assembly, I don't need to keep it to the top, and I don't actually need to find a top port MIC!

I decided to add two of them for stereo!

I polished the README some more, and I looked over the repository one more time to make sure everything was in order. This is sort of the final opportunity to ship something, so...
![image](https://cdn.hackclub.com/01a01a24-0567-7a83-b8d2-2171645e7360/image.png)
![image](https://cdn.hackclub.com/01a01a21-ee29-7ffb-8f21-385b3966900b/image.png)


# log #15: final final touches (PCB)
![image](https://cdn.hackclub.com/01a013e8-a88b-70a7-926b-f052c556bbe9/image.png)
Absolute final iteration of the PCB! Added silkscreen art on the back, and I replaced a lot of the original KiCad silkscreen labels with labels I typed up in Canva and turned into SVGs in Inkscape.
![image](https://cdn.hackclub.com/01a013e9-212e-7bb5-abda-a71e55d434a3/image.png)
![image](https://cdn.hackclub.com/01a013ea-6939-7a6a-aee3-28b6c38c70b6/image.png)
![image](https://cdn.hackclub.com/01a013ea-fde7-7084-808b-17984540565e/image.png)


# log #16: jlc pcb wrangling
![image](https://cdn.hackclub.com/01a014dc-6510-7dc3-9907-ced341ddcc08/image.png)
So this is horrific. I do love tariffs.
![image](https://cdn.hackclub.com/01a014dd-32b5-7b5d-9f18-b3b8f0c59a6c/image.png)
Fixed up the pick and place, so that took a while. I'm not sure why all the components are always initially out of alignment.


# log #17: wrangling bom + setting up github + readme
![image](https://cdn.hackclub.com/01a014ec-8ed8-71cf-980f-a82765fcf3ec/image.png)
![image](https://cdn.hackclub.com/01a014ed-55a8-789c-9167-0a6f198eb968/image.png)
Done setting up the BOM! I did not think the PCB would be so expensive, but it's mostly PCBA and shipping so... Once I have access to a reflow oven and a congressperson, costs might go down. I am getting multiple copies of the parts on aliexpress because I'm not sure if all of them will work.

Also have no clue how backordering works, and I could not find the mic on aliexpress or amazon, so I might have to buy off digikey. Leaving a bit of extra grant money for that + sudden shipping increases + contingencies with parts/PCB. I am insanely worried about having to hand solder the mic down. I think I might ruin a few mics in the process. 


# log #18: README finished
![image](https://cdn.hackclub.com/01a01829-161e-7995-9979-e1964451acfd/image.png)
Finally finished up the README! Also added a lot of finishing touches to the repository. This took so long, but I'm so happy with it. It's beautiful. It's that thing about eating your own cooking, I think. yayyy!


# log #19: REALIZED I NEED AN SD CARD MODULE
oml this could have been bad. So I completely forgot that a sampler synth really needs an sd card, or at least the option for one, so I should add the header pin ports, just in case. The daisy seed has enough internal ram for a little bit of sampling, so it's optional, but most samplers have it to upload and store samples.
![image](https://cdn.hackclub.com/01a01a0d-afab-7b7e-934d-d3c0832abf80/3D_back.png)
on the bright side, or the dark side here ig, changing the background color in 3d viewing worked! so it looks a little cleaner now. unfortunately the back looks so much worse with the sd card module. but that can be painted over! And it really is optional should you want extra storage. I'm not going to be adding it initially, so it's not going in the BOM. I'll buy it myself if I eventually want it.


# log #20: Last One, Really
Completely finished the README. Everything's on github now, including the journals. I'm doneeeee. Yayyyyyy!