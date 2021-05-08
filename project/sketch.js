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
	const sampleRate = 44100;
		
	// Graph position
	const graphStartX = 0.1;
	const graphStartY = 0.05;
	const graphEndX = 0.95;
	const graphEndY = 0.5;
	const graphLengthX = graphEndX - graphStartX;
	const graphLengthY = graphEndY - graphStartY;
	
	// Graph parameters
	const freqMax = 20000;
	const freqStep = 2000;
	const dbMax = 20;
	const dbMin = -30;
	const dbStep = 10;
	const dbRange = dbMax - dbMin;

	p.setup = function() {
		p.createCanvas(window.innerWidth, window.innerHeight);
		p.colorMode(p.RGB, 1);
	};

	p.draw = function() {
		// Get the data buffer(s) from the Bela C++ program
		var buffers = Bela.data.buffers;
		
		// Check if any data has been received
		if(!buffers.length) return;
		
		// Calculate the FFT size
		var fftSize = buffers[0].length;
		
		// White background
		p.background(255);
		
		// Draw graph box
		p.noFill();
		p.stroke(0, 0, 0, 0.3);
		p.strokeWeight(0.8);
		p.beginShape();
		p.vertex(p.windowWidth * graphStartX, p.windowHeight * graphStartY);
		p.vertex(p.windowWidth * graphEndX, p.windowHeight * graphStartY);
		p.vertex(p.windowWidth * graphEndX, p.windowHeight * graphEndY);
		p.vertex(p.windowWidth * graphStartX, p.windowHeight * graphEndY);
		p.vertex(p.windowWidth * graphStartX, p.windowHeight * graphStartY);
		p.endShape();
		
		// Draw axis titles
		p.noStroke();
		p.fill(0);
		p.text("Frequency [kHz]", (graphStartX + graphLengthX/2) * p.windowWidth, graphEndY * p.windowHeight + 20);
		
		// Draw a line for each of the buffers received
		for(let k = 0; k < 1; k += 1)
		{	
			// Select color
			let rem = k % 3;
			let color = p.color(0 === rem, 1 == rem, 2 == rem);
			
			// Draw line
			p.noFill();
			p.stroke(color);
			p.beginShape();
			let buf = buffers[k];
			for (let i = 0; i < fftSize && i < buf.length; i++) {
				// Position
				let dbVal = 20*(Math.log10(buf[i]));
				let yRel = (dbVal - dbMin) / dbRange;
				if (yRel < 0) yRel = 0;
				if (yRel > 1) yRel = 1;
				let xRel = i / (fftSize * freqMax/(sampleRate/2));
				
				if (xRel > 1) break;
				
				let yPos = graphStartY + graphLengthY * (1 - yRel);
				let xPos = graphStartX + graphLengthX * xRel;
				
				// Draw point
				p.vertex(p.windowWidth * xPos, p.windowHeight * yPos);
			}
			p.endShape();
		}

		// Draw Y grid
		for(let db = dbMax; db >= dbMin; db -= dbStep)
		{
			p.stroke(0, 0, 0, 0.3);
			p.strokeWeight(0.2);
			let yPos = graphStartY + graphLengthY * (1 - (db - dbMin) / dbRange);
			p.line(p.windowWidth * graphStartX, yPos * p.windowHeight, p.windowWidth * graphEndX, yPos * p.windowHeight);
			
			p.noStroke();
			p.fill(0);
			let txt = db.toFixed(1)+'dB';
			p.text(txt, graphStartX * p.windowWidth - 45, yPos * p.windowHeight);
		}

		// Draw X grid
		for(let freq = 0; freq <= freqMax; freq += freqStep)
		{
			p.stroke(0, 0, 0, 0.3);
			p.strokeWeight(0.2);
			let xPos = graphStartX + graphLengthX * freq / freqMax;
			p.line(xPos * p.windowWidth, p.windowHeight * graphStartY, xPos * p.windowWidth, p.windowHeight * graphEndY);
			let txt = (freq / 1000).toFixed(1);
			p.noStroke();
			p.fill(0);
			p.text(txt, xPos * p.windowWidth, graphEndY * p.windowHeight + 10);
		}
	};

	p.windowResized = function() {
		p.resizeCanvas(window.innerWidth, window.innerHeight);
	};
}, 'gui');
