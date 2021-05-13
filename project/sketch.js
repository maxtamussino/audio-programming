/*
 ____  _____ _        _    
| __ )| ____| |      / \   
|  _ \|  _| | |     / _ \  
| |_) | |___| |___ / ___ \ 
|____/|_____|_____/_/   \_\

http://bela.io

C++ Real-Time Audio Programming with Bela - Lecture 17: Block-based processing
*/

// This file implements a browser-based GUI using p5.js
// It draws a spectrum of the signal received from the Bela program
// It is based on the Bela example 'Gui/frequency-response' which has
// a number of additional features and controls.

var guiSketch = new p5(function( p ) {
	// Global variables
	const sampleRate = 44100 / 16;
	const textLineDistance = 30;
	
	// Holding paused info
	var graphPaused = false;
	var spectrumBuffer, detectedFundamentalFreq, detectedFundamentalMIDI;
		
	// Graph position
	const graphStartX = p.windowWidth  * 0.1;
	const graphStartY = p.windowHeight * 0.05;
	const graphEndX =   p.windowWidth  * 0.95;
	const graphEndY =   p.windowHeight * 0.7;
	const graphLengthX = graphEndX - graphStartX;
	const graphLengthY = graphEndY - graphStartY;
	
	// Settings
	const settingsSectionTitles = ["Frequency", "Magnitude"];
	const settingsTitles = [["Min", "Max", "Steps"],
						    ["Min", "Max", "Steps"]];
	const settingsMins = [[0, 0, 1],
						  [-50, -30, 1]];
	const settingsMaxs = [[20000, 20000, 20],
						  [30, 50, 10]];
	const settingsStd = [[0, 1000, 10],
						  [-20, 40, 6]];
	var   settingsCurr = [[0, 1000, 10],
						  [-20, 40, 6]];
	var   settingsInputs = [[],[]];
	
	// Settings section parameters
	const settingsStartX = graphStartX;
	const settingsStartY = graphEndY + 30;
	const settingsInputBoxLength = 50;
	const settingsSectionDist = 150;
	const settingsTextLen = 0;
	
	// Results section parameters
	const resultsStartX = graphStartX + 300;
	const resultsStartY = graphEndY + 30;
	
	// Tuning section parameters
	const tuningLineLength = 150;
	const tuningFrameHeight = 20;
	const tuningStartX = graphEndX - tuningLineLength;
	const tuningStartY = graphEndY + 30;

	p.setup = function() { 
		p.createCanvas(window.innerWidth, window.innerHeight);
		p.colorMode(p.RGB, 1);
		
		let set_idx_max = 0;
		for (let sec = 0; sec < settingsSectionTitles.length; sec++) {
			for (let set_idx = 0; set_idx < settingsTitles[sec].length; set_idx++) {
				create_setting(sec, set_idx);
				if (set_idx > set_idx_max) set_idx_max = set_idx;
			}
		}
		
		// Button position
		let xPos = settingsStartX - 50;
		let yPos = settingsStartY + (set_idx_max + 2) * textLineDistance;
		
		// Reset button
		button_reset = p.createButton('Reset');
		button_reset.position(xPos, yPos);
		button_reset.mousePressed(reset_all_settings);
		
		// Set button
		button_set = p.createButton('Set');
		button_set.position(xPos + 10 + button_reset.width, yPos);
		button_set.mousePressed(set_all_settings);
		
		// Pause/resume button
		button_pause = p.createButton('Pause/resume');
		button_pause.position(xPos + 20 + button_reset.width + button_set.width, yPos);
		button_pause.mousePressed(function() { graphPaused = ! graphPaused;});
	};
	
	function create_setting(sec, set_idx) {
		// Position
		let xPos = settingsStartX + sec * settingsSectionDist;
		let yPos = settingsStartY + (set_idx + 1) * textLineDistance;
		
		// Input text field
		input = p.createInput(p.str(settingsStd[sec][set_idx]));
		input.position(xPos + settingsTextLen, yPos);
		input.size(settingsInputBoxLength);
		settingsInputs[sec][set_idx] = input;
	}
	
	
	function set_setting(sec, set_idx) {
		var num = parseInt(settingsInputs[sec][set_idx].value());
		const min = settingsMins[sec][set_idx];
		const max = settingsMaxs[sec][set_idx];
		if (isNaN(num)) {
			settingsInputs[sec][set_idx].value(settingsCurr[sec][set_idx]);
			return;
		} else if (num < min) {
			num = min;
		} else if (num > max) {
			num = max;
		}
		settingsCurr[sec][set_idx] = num;
	}
	
	function reset_all_settings() {
		set_all_settings(true);
	}
	
	function set_all_settings(reset) {
		for (let sec = 0; sec < settingsSectionTitles.length; sec++) {
			for (let set_idx = 0; set_idx < settingsTitles[sec].length; set_idx++) {
				if (reset) {
					settingsCurr[sec][set_idx] = settingsStd[sec][set_idx];
					settingsInputs[sec][set_idx].value(settingsStd[sec][set_idx]);
				} else {
					set_setting(sec, set_idx);
				}
			}
		}
	}
	
	function index_to_freq(index, fftsize) {
		// For the parameter "fftsize", twice the number of received FFT bins
		// have to be given. This is because only half of the obtained FFT amplitude
		// results is unique, and only half is transmitted.
		return index * sampleRate / fftsize;
	}
	
	function freq_to_text(freq) {
		let txt = "";
		if (freq > 1000) {
			txt += (freq/1000).toFixed(3) + " kHz";
		} else {
			txt += freq.toFixed(2) + " Hz";
		}
		return txt;
	}
	
	function midi_to_text(midi) {
		const notes_str = ["A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"];
		
		// Check if inside english note notation
		let midi_reduced = midi - 21;
		if (midi_reduced < 0) {
			return "";
		}
		
		// Calculate note notation
		let note = midi_reduced % 12;
		let octave = Math.floor(midi_reduced/12);
		let ret_str = "" + notes_str[note] + octave;
		
		// If the note includes "#", it may also be displayed as the higher note using "b"
		if (ret_str.includes("#")) ret_str += "/" + notes_str[(note + 1) % 12] + "b" + octave;
		return ret_str;
	}

	p.draw = function() {
		// Only retrieve new infos if graph not paused
		if (!graphPaused) {
			// Get the data buffer(s) from the Bela C++ program
			const buffers = Bela.data.buffers;
			
			// Check if any data has been received
			if(!buffers.length) return;
			
			// Get buffer infos
			spectrumBuffer = buffers[0];
			detectedFundamentalFreq = parseFloat(buffers[1]);
			detectedFundamentalMIDI = parseFloat(buffers[2]);
		}
		
		// Interpretation of the buffer info
		const fftNumberOfBins = spectrumBuffer.length;
		
		// Read settings
		const freqMin = settingsCurr[0][0];
		const freqMax = settingsCurr[0][1];
		const freqRange = freqMax - freqMin;
		const freqStep = freqRange / settingsCurr[0][2];
		const magMin = settingsCurr[1][0];
		const magMax = settingsCurr[1][1];
		const magRange = magMax - magMin;
		const magStep = magRange / settingsCurr[1][2];
		
		
		// Start graph
		p.background(255);
		p.push();
		p.translate(graphStartX, graphStartY);
		
		// Draw graph box
		p.noFill();
		p.stroke(0, 0, 0, 0.3);
		p.strokeWeight(0.8);
		p.beginShape();
		p.vertex(0, 0);
		p.vertex(graphLengthX, 0);
		p.vertex(graphLengthX, graphLengthY);
		p.vertex(0, graphLengthY);
		p.vertex(0, 0);
		p.endShape();
		
		// Draw axis titles and graph paused info
		p.push();
		p.noStroke();
		p.fill(0);
		p.textAlign(CENTER);
		let xLabel = "Frequency [";
		if (freqMax > 1000) xLabel += "k";
		xLabel += "Hz]";
		p.text(xLabel, graphLengthX / 2, graphLengthY + 30);
		
		p.push();
		p.translate(-40, graphLengthY/2);
		p.rotate(radians(270));
		p.text("Magnitude [dB]", 0, 0);
		p.pop();
		
		if (graphPaused) {
			p.textAlign(RIGHT, TOP);
			p.text("Graph paused", graphLengthX - 10, 10);
		}
		p.pop();
		
		// Draw graph line
		p.push();
		p.noFill();
		p.stroke(1,0,0);
		p.beginShape();
		for (let i = 0; i < fftNumberOfBins && i < spectrumBuffer.length; i++) {
			// X Position
			let freqVal = index_to_freq(i, fftNumberOfBins * 2);
			let xRel = (freqVal - freqMin) / freqRange;
			if (xRel < 0) continue; // Below minimum, try next point
			if (xRel > 1) break;    // Above maximum, end graph
			
			// Y Position
			let magVal = 20*(Math.log10(spectrumBuffer[i]));
			let yRel = (magMax - magVal) / magRange;
			if (yRel < 0) yRel = 0;
			if (yRel > 1) yRel = 1;
			
			// Draw point
			p.vertex(graphLengthX * xRel, graphLengthY * yRel);
		}
		p.endShape();
		p.pop();

		// Draw Y grid
		p.push();
		p.textAlign(RIGHT, CENTER);
		for(let db = magMax; db >= magMin; db -= magStep) {
			// Draw the line
			p.stroke(0, 0, 0, 0.3);
			p.strokeWeight(0.2);
			let yPos = graphLengthY * (magMax - db) / magRange;
			p.line(0, yPos, graphLengthX, yPos);
			
			// Draw label
			p.noStroke();
			p.fill(0);
			p.text(db.toFixed(1), -10, yPos);
		}
		p.pop();
		
		// Draw X grid
		p.push();
		p.textAlign(CENTER);
		p.stroke(0, 0, 0, 1);
		p.strokeWeight(0.2);
		let xPosFundamental = graphLengthX * (detectedFundamentalFreq - freqMin) / freqRange;
		p.line(xPosFundamental, 0, xPosFundamental, graphLengthY);
		for(let freq = freqMin; freq <= freqMax; freq += freqStep)
		{
			// Draw line
			p.stroke(0, 0, 0, 0.3);
			p.strokeWeight(0.2);
			let xPos = graphLengthX * (freq - freqMin) / freqRange;
			p.line(xPos, 0, xPos, graphLengthY);
			
			// Draw label
			p.noStroke();
			p.fill(0);
			let freqLabelScale = 1;
			if (freqMax > 1000) freqLabelScale = 1000;
			p.text((freq / freqLabelScale).toFixed(2), xPos, graphLengthY + 15);
		}
		p.pop();
		
		
		// End graph drawing and start settings
		p.pop();
		p.push();
		p.translate(settingsStartX, settingsStartY);
		
		// Draw settings descriptions
		p.push();
		p.noStroke();
		p.fill(0);
		p.textAlign(LEFT, CENTER);
		p.textSize(14);
		p.translate(0, 10);
		for (let sec = 0; sec < settingsTitles.length; sec++) {
			let xPos = sec * settingsSectionDist;
			p.textStyle(BOLD);
			p.text(settingsSectionTitles[sec], xPos, 0);
			p.textStyle(NORMAL);
			for (let set_idx = 0; set_idx < settingsTitles[sec].length; set_idx++) {
				let yPos = (set_idx + 1) * textLineDistance;
				p.text(settingsTitles[sec][set_idx], xPos, yPos);
			}
		}
		p.pop();
		
		
		// End settings drawing and start results drawing
		p.pop();
		p.push();
		p.translate(resultsStartX, resultsStartY);
		
		// Draw results
		p.push();
		p.noStroke();
		p.fill(0);
		p.textAlign(LEFT, CENTER);
		p.textSize(14);
		
		p.textStyle(BOLD);
		p.translate(0,10);
		p.text("Results", 0, 0);
		p.textStyle(NORMAL);
		
		p.translate(0, textLineDistance);
		let detectedFundamentalText = "Fundamental: " + freq_to_text(detectedFundamentalFreq);
		p.text(detectedFundamentalText, 0, 0);
		
		p.translate(0, textLineDistance);
		p.text("MIDI note: " + detectedFundamentalMIDI.toFixed(2), 0, 0);
		p.pop();
		
		
		// End results drawing and start tuning drawing
		p.pop();
		p.push();
		p.translate(tuningStartX, tuningStartY);
		
		// Heading
		p.push();
		p.noStroke();
		p.fill(0);
		p.textAlign(CENTER, CENTER);
		p.textSize(14);
		p.textStyle(BOLD);
		p.translate(0,10);
		p.text("Tuning", 0, 0);
		p.textStyle(NORMAL);
		
		// Note (english notation)
		p.translate(0, 1.5 * textLineDistance);
		p.textSize(40);
		let nearestMIDI = parseInt(Math.round(detectedFundamentalMIDI));
		p.text(midi_to_text(nearestMIDI), 0, 0);
		
		// Easier calculation of coordinates
		const right = tuningLineLength;
		const left = -tuningLineLength;
		const up = -tuningFrameHeight/2;
		const down = tuningFrameHeight/2;
		const rectScaling = 0.7;
		
		// Good/bad coloring
		p.translate(0, 1.5 * textLineDistance);
		p.push();
		p.rectMode(CORNERS);
		p.fill(0, 255, 0, 200);
		p.rect(left/2 * 0.2, up * rectScaling, right/2 * 0.2, down * rectScaling);
		p.fill(255, 255, 0, 200);
		p.rect(left/2 * 0.5, up * rectScaling, left/2 * 0.2, down * rectScaling);
		p.rect(right/2 * 0.5, up * rectScaling, right/2 * 0.2, down * rectScaling);
		p.fill(255, 0, 0, 200);
		p.rect(left/2, up * rectScaling, left/2 * 0.5, down * rectScaling);
		p.rect(right/2, up * rectScaling, right/2 * 0.5, down * rectScaling);
		p.pop();
		
		// Frame lines
		p.stroke(0);
		p.line(0, 2*up, 0, 2*down);
		p.line(right, down, right, up);
		p.line(right/2, down, right/2, up);
		p.line(left, down, left, up);
		p.line(left/2, down, left/2, up);
		
		// Next notes (english notation)
		p.textSize(16);
		p.textStyle(NORMAL);
		p.text(midi_to_text(nearestMIDI + 1), tuningLineLength, -tuningFrameHeight/2 - 10);
		p.text(midi_to_text(nearestMIDI - 1), -tuningLineLength, -tuningFrameHeight/2 - 10);
		
		// Tuning line
		let linelen = (detectedFundamentalMIDI - nearestMIDI) * tuningLineLength;
		p.line(linelen, up, linelen, down);
		p.strokeWeight(7);
		p.line(0, 0, linelen, 0);
		p.pop();
		
		// End tuning drawing
		p.pop();
	};

	p.windowResized = function() {
		p.resizeCanvas(window.innerWidth, window.innerHeight);
	};
}, 'gui');
