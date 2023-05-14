/*
 * Copyright (c) 2017-2023  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

var easeOut = function(start, stop, fraction) {
  return start + Math.round((1 - Math.pow(1 - fraction, 2)) * (stop - start));
}

/*
  Note:  v0, v1, and duration should all be either positive or negative
  
  v0	:	initial velocity
  v1	:	terminating velocity
  k	:	coefficient of kinetic friction
*/
class ScrollerMode {
  constructor() {
    this.absV1 = 0.005;
    this.k = 0.0015;
  }
  bind(behavior, scroller) {
    behavior.mode = this;
  }
  computeDistance(v0, v1, k) {
    return (v0 - v1) / k
  }
  computeDuration(v0, v1, k) {
    return Math.log(v0 / v1) / k
  }
  computePrediction(behavior, coordinate) {
    var range = behavior.range;
    var size = behavior.size;
    range -= size;
    if (behavior.direction > 0) {
      coordinate += size;
    }
    else {
      coordinate -= size;
    }
    if (coordinate < 0)
      coordinate = 0;
    else if (coordinate > range)
      coordinate = range;
    return coordinate;
  }
  evaluateBounceback(v0, duration, time) {
    if (time >= duration)	
      return 0
    else if (time >= 0)		
      return v0 * time * Math.exp(-10.0 * time / duration)
    else 													
      return NaN											
  }
  evaluatePositionAtTime(v0, v1, k, t) {
    if (t >= this.computeDuration(v0, v1, k))
      return this.computeDistance(v0, v1, k)
    else if (t <= 0)					
      return 0
    else								
      return (1.0 - Math.exp(-k * t)) * v0 / k
  }
  evaluateTimeAtPosition(v0, v1, k, position) {
    var distance = this.computeDistance(v0, v1, k)
    if (Math.abs(position) > Math.abs(distance))
      return NaN
    else if (position == 0)								
      return NaN
    else												
      return - Math.log(1.0 - k * position / v0) / k
  }
  evaluateVelocityAtTime(v0, v1, k, time) {
    if (time >= this.computeDuration(v0, v1, k))
      return 0
    else if (time < 0)						
      return 0
    else								
      return Math.exp(-k * time) * v0
  }
  onFinished(behavior, scroller) {
  }
  onTimeChanged(behavior, scroller) {
  }
  onTouchBegan(behavior, scroller, id, x, y, ticks, samples) {
  }
  onTouchCancelled(behavior, scroller, id, x, y, ticks) {
    scroller.scroll = scroller.constraint;
    scrollerStillMode.bind(behavior, scroller);
  }
  onTouchEnded(behavior, scroller, id, x, y, ticks, samples) {
  }
  onTouchMoved(behavior, scroller, id, x, y, ticks, samples) {
  }
};

class ScrollerStillMode extends ScrollerMode {
  bind(behavior, scroller) {
    behavior.boost = 0.5;
    behavior.direction = 0;
    behavior.mode = this;
    behavior.bouncing = false;
	behavior.first = behavior.last = null;
    if (scroller.tracking) {
      scroller.tracking = false;
      scroller.distribute("onScrolled");
      scroller.bubble("onTouchScrolled");
    }
  }
  onTouchBegan(behavior, scroller, id, x, y, ticks, samples) {
    if (scroller.running) {
      scroller.stop();
      behavior.scrollTo(scroller, behavior.stop);
    }
    behavior.anchor = behavior.selectParameter(x, y);
    behavior.antiAnchor = behavior.selectParameter(y, x);
    behavior.boost *= 2;
    behavior.first = behavior.last = { previous: null, coordinate: behavior.selectParameter(x, y), ticks: ticks };
    behavior.range = behavior.selectSize(scroller.first.size);
    behavior.size = behavior.selectSize(scroller.size);
    behavior.start = behavior.stop = behavior.selectPosition(scroller.scroll);
  }
  onTouchMoved(behavior, scroller, id, x, y, ticks, samples) {
    var delta = Math.abs(behavior.selectParameter(x, y) - behavior.anchor);
    var antiDelta = Math.abs(behavior.selectParameter(y, x) - behavior.antiAnchor);
    if ((delta > antiDelta) && (delta > 8)) {
      if (behavior.captureTouch(scroller, id, x, y, ticks)) {
        scroller.tracking = true;
        scrollerScrollMode.bind(behavior, scroller);
        scrollerScrollMode.onTouchMoved(behavior, scroller, id, x, y, ticks, samples);
      }
    }
  }
  onTimeChanged(behavior, scroller) {
    var fraction = scroller.fraction;
    behavior.scrollTo(scroller, easeOut(behavior.start, behavior.stop, fraction));
  }
  onFinished(behavior, scroller) {
    if (scroller.tracking) {
      scroller.tracking = false;
      scroller.distribute("onScrolled");
      behavior.scrollTo(scroller, behavior.stop);
      scroller.bubble("onTouchScrolled");
    }
  }
}

const scrollerStillMode = Object.freeze(new ScrollerStillMode());

class ScrollerScrollMode extends ScrollerMode {
  onTouchEnded(behavior, scroller, id, x, y, ticks, samples) {
    this.onTouchMoved(behavior, scroller, id, x, y, ticks, samples);
    scrollerTossMode.bind(behavior, scroller);
  }
  onTouchMoved(behavior, scroller, id, x, y, ticks, samples) {
    //var points = behavior.peek(id);
	for (let sample = behavior.last; sample; sample = sample.previous) {
		if (sample.ticks < (ticks - 500)) {
			behavior.first = sample;
			sample.previous = null;
			break;
		}
	}
    var c = samples.length;
    for (var i = 0; i < c; i++) {
      var point = samples.peek(i);
      var coordinate = behavior.selectParameter(point.x, point.y);
      ticks = point.ticks;
      if (behavior.last.coordinate != coordinate) {
        var direction = (coordinate > behavior.last.coordinate) ? -1 : 1
        if (behavior.direction != direction) {
          behavior.boost = 1;
          behavior.direction = direction;
          behavior.first = behavior.last = { previous: null, coordinate: coordinate, ticks: ticks };
        }
      }
      behavior.last = { previous: behavior.last, coordinate: coordinate, ticks: ticks };
    }
    behavior.stop = behavior.start - behavior.last.coordinate + behavior.anchor;
    behavior.scrollTo(scroller, behavior.stop);
    //behavior.predictTo(scroller, this.computePrediction(behavior, behavior.stop));
  }
};

const scrollerScrollMode = Object.freeze(new ScrollerScrollMode());

class ScrollerTossMode extends ScrollerMode {
  bind(behavior, scroller) {
    var speed = 0;
    var start = behavior.stop;
    var stop;
    if (scroller.looping)
      stop = start;
    else
      stop = behavior.selectPosition(scroller.constraint);
    if (start == stop) {
      var delta = 0;
      var duration = 60;
      var former = behavior.last;
      var coordinate = former.coordinate;
      var ticks = former.ticks - duration;
      var sample = behavior.first;
      if (ticks < sample.ticks) {
        delta = coordinate - sample.coordinate;
        speed = delta / (former.ticks - sample.ticks);
      }
      else {
        sample = former.previous;
        while (sample) {
          if (ticks >= sample.ticks) {
            delta = (coordinate - sample.coordinate) + ((former.coordinate - sample.coordinate) * (ticks - sample.ticks) / (former.ticks - sample.ticks))
            speed = delta / duration;
            break;
          }
          former = sample;
          sample = former.previous;
        }
      }
      if (Math.abs(delta) > 20)
        speed *= behavior.boost
      else
        speed = 0;
    }	
    if (speed) {
      behavior.mode = this;
      behavior.v0 = speed
      behavior.v1 = (speed < 0) ? -this.absV1 : this.absV1
      var scroll = start - this.computeDistance(behavior.v0, behavior.v1, this.k);
      var stop = behavior.snap(scroller, scroll, scroll - start);
      var duration;
      if (scroll != stop)
        duration = this.evaluateTimeAtPosition(behavior.v0, behavior.v1, this.k, start - stop);
      else
        duration = this.computeDuration(behavior.v0, behavior.v1, this.k);
      if (duration > 0) {
        behavior.start = start;
        behavior.stop = stop;
        behavior.time = 0;
        scroller.duration = duration;
        scroller.time = 0;
        scroller.start();
      }
      else {
        this.onFinished(behavior, scroller);
      }
    }
    else {
      this.onFinished(behavior, scroller);
    }
  }
  onFinished(behavior, scroller) {
    scrollerBounceMode.bind(behavior, scroller);
  }
  onTouchBegan(behavior, scroller, id, x, y, ticks, samples) {
    var scroll = behavior.selectPosition(scroller.scroll);			
    var stop = behavior.snap(scroller, behavior.selectPosition(scroller.scroll), behavior.direction);
    if (scroll != stop) {
      if (behavior.captureTouch(scroller, id, x, y, ticks)) {
        behavior.boost = 0.5;
        behavior.direction = 0;
        
        behavior.first = behavior.last = { previous: null, coordinate: behavior.selectParameter(x, y), ticks: ticks };
        behavior.range = behavior.selectSize(scroller.first.size);
        behavior.size = behavior.selectSize(scroller.size);
        behavior.start = behavior.stop = behavior.selectPosition(scroller.scroll);
    
        behavior.anchor = behavior.selectParameter(x, y);
        behavior.antiAnchor = behavior.selectParameter(y, x);
        scroller.tracking = true;
        scrollerScrollMode.bind(behavior, scroller);
        scrollerScrollMode.onTouchMoved(behavior, scroller, id, x, y, ticks, samples);
      }
    }
    else {						
      scroller.stop();
      var boost = behavior.boost;
      scrollerStillMode.bind(behavior, scroller);
      behavior.boost = boost;
      scrollerStillMode.onTouchBegan(behavior, scroller, id, x, y, ticks, samples);
    }
  }
  onTimeChanged(behavior, scroller) {
    var time = scroller.time;
    var position = this.evaluatePositionAtTime(behavior.v0, behavior.v1, this.k, time);
    behavior.scrollTo(scroller, behavior.start - position);
    if (!scroller.looping) {
      var scroll = behavior.selectPosition(scroller.scroll);
      var constraint = behavior.selectPosition(scroller.constraint);
      if (scroll != constraint) {
        scroller.stop();
        position = behavior.start - constraint;
        time = this.evaluateTimeAtPosition(behavior.v0, behavior.v1, this.k, position);
        behavior.v0 = this.evaluateVelocityAtTime(behavior.v0, behavior.v1, this.k, time);
        behavior.stop = behavior.start = constraint;
        scrollerElasticMode.bind(behavior, scroller);
        return;
      }
    }
    var delta = time - behavior.time;
    behavior.time = time;
    //time += delta;
    //coordinate = behavior.start - this.evaluatePositionAtTime(behavior.v0, behavior.v1, this.k, time);
    //behavior.predictTo(scroller, this.computePrediction(behavior, coordinate));
  }
}

const scrollerTossMode = Object.freeze(new ScrollerTossMode());

class ScrollerElasticMode extends ScrollerTossMode {
  bind(behavior, scroller) {
    behavior.mode = this;
    scroller.duration = 1000;
    scroller.time = 0;
    scroller.start();
  }
  onFinished(behavior, scroller) {
    scrollerStillMode.bind(behavior, scroller);
  }
  onTimeChanged(behavior, scroller) {
    var position = this.evaluateBounceback(behavior.v0, scroller.duration, scroller.time) 
    position = (position > 0) ? Math.floor(position) : Math.ceil(position)
    behavior.scrollTo(scroller, behavior.start - position);
  }
}

const scrollerElasticMode = Object.freeze(new ScrollerElasticMode());

class ScrollerBounceMode extends ScrollerTossMode {
  bind(behavior, scroller) {
    var start = behavior.stop;
    var stop;
    if (scroller.looping)
      stop = behavior.snap(scroller, start, 0);
    else
      stop = behavior.snap(scroller, behavior.selectPosition(scroller.constraint), 0);
    if (start != stop) {
      behavior.mode = this;
      behavior.start = start;
      behavior.stop = stop;
      scroller.duration = 500;
      scroller.time = 0;
      scroller.start();
    }
    else 
      this.onFinished(behavior, scroller);
  }
  onFinished(behavior, scroller) {
    scrollerStillMode.bind(behavior, scroller);
  }
  onTimeChanged(behavior, scroller) {
    var fraction = scroller.fraction;
    behavior.scrollTo(scroller, easeOut(behavior.start, behavior.stop, fraction));
  }
}

const scrollerBounceMode = Object.freeze(new ScrollerBounceMode());

class ScrollerBehavior extends Behavior {
  captureTouch(scroller, id, x, y, ticks) {
    scroller.captureTouch(id, x, y, ticks);
    return true;
  }
  onCreate(scroller, data) {
    this.data = data;
    scrollerStillMode.bind(this, scroller);
  }
  onDisplaying(scroller) {
    if ("scroll" in this.data)
      scroller.scroll = this.data.scroll;
  }
  onFinished(scroller) {
    this.mode.onFinished(this, scroller);
  }
  onMouseScrolled(scroller, dx, dy) {
    scroller.scrollBy(dx, dy);
  }
  onTimeChanged(scroller) {
    this.mode.onTimeChanged(this, scroller);
  }
  onTouchBegan(scroller, id, x, y, ticks, samples) {
    this.mode.onTouchBegan(this, scroller, id, x, y, ticks, samples);
  }
  onTouchCancelled(scroller, id, x, y, ticks) {
    this.mode.onTouchCancelled(this, scroller, id, x, y, ticks);
  }
  onTouchEnded(scroller, id, x, y, ticks, samples) {
    this.mode.onTouchEnded(this, scroller, id, x, y, ticks, samples);
  }
  onTouchMoved(scroller, id, x, y, ticks, samples) {
    this.mode.onTouchMoved(this, scroller, id, x, y, ticks, samples);
  }
  onUndisplaying(scroller) {
    if ("scroll" in this.data)
      this.data.scroll = scroller.scroll;
  }
  snap(scroller, position, direction) {
    return position;
  }
}

export class HorizontalScrollerBehavior extends ScrollerBehavior {
  predictTo(scroller, coordinate) {
    scroller.predictTo(coordinate, 0);
  }
  scrollBy(scroller, coordinate) {
    scroller.scrollBy(coordinate, 0);
  }
  scrollTo(scroller, coordinate) {
    scroller.scrollTo(coordinate, 0);
  }
  selectParameter(x, y) {
    return x;
  }
  selectPosition(position) {
    return position.x;
  }
  selectSize(size) {
    return size.width;
  }
}

export class VerticalScrollerBehavior extends ScrollerBehavior {
  predictTo(scroller, coordinate) {
    scroller.predictTo(0, coordinate);
  }
  scrollBy(scroller, coordinate) {
    scroller.scrollBy(0, coordinate);
  }
  scrollTo(scroller, coordinate) {
    scroller.scrollTo(0, coordinate);
  }
  selectParameter(x, y) {
    return y;
  }
  selectPosition(position) {
    return position.y;
  }
  selectSize(size) {
    return size.height;
  }
}	

