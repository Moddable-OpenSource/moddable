/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

'use strict';
var Alexa = require('alexa-sdk');
var http = require('http');

var APP_ID = undefined; //OPTIONAL: replace with "amzn1.echo-sdk-ams.app.[your-unique-value-here]";
var SKILL_NAME = 'Moddable Relay';

var GREETING = "Welcome to the Moddable relay app. You can control your outlets using this skill.";

exports.handler = function(event, context, callback) {
    var alexa = Alexa.handler(event, context);
    alexa.APP_ID = APP_ID;
    alexa.registerHandlers(handlers);
    alexa.execute();
};

function doHTTP(one, two, thing, message){
  let URL = "http://somethingevil.net/relay.php?";
  if (one !== undefined){
    URL += "one=" + one + "&";
  }
  if (two !== undefined){
    URL += "two=" + two;
  }
  http.get(URL, function (result) {
    console.log('Success, with: ' + result.statusCode);
    thing.emit(":tell", message);
  }).on('error', function (err) {
    console.log('Error, with: ' + err.message);
    thing.emit(":tell", "Sorry, I failed to " + message);
  });
}

function doStatus(thing, outlet){
  let URL = "http://somethingevil.net/relay.php";
  http.get(URL, function (result) {
    result.on("data", function(chunk) {
      let schunk = chunk.toString();
      console.log("BODY: *" + schunk + "*");
      
      let part1 = "Outlet one is  " + (schunk.charAt(0) == "0" ? "off" : "turned on") + "...";
      let part2 = "Outlet two is  " + (schunk.charAt(1) == "0" ? "off" : "turned on") + "...";
      if (outlet === undefined){
        thing.emit(":tell", part1 + " " + part2);
      }else if (parseInt(outlet) == 1){
        thing.emit(":tell", part1);
      }else if (parseInt(outlet) == 2){
        thing.emit(":tell", part2);
      }else{
        thing.emit(":tell", "Sorry, " + outlet + " is not a valid outlet number.");
      }
    });
  }).on('error', function (err) {
    console.log('Error, with: ' + err.message);
    thing.emit(":tell", "Sorry, I failed to retreive the relay status.");
  });
}

var handlers = {
    'LaunchRequest': function () {
      this.emit(':tell', GREETING);
    },
    'ToggleOutlet': function () {
        let outlet = this.event.request.intent.slots.Outlet.value;
        if (outlet === undefined){
          doHTTP("toggle", "toggle", this, "Toggling both outlets.");
        }else if (parseInt(outlet) == 1){
          doHTTP("toggle", undefined, this, "Toggling outlet one.");
        }else if (parseInt(outlet) == 2){
          doHTTP(undefined, "toggle", this, "Toggling outlet two.");
        }else{
          this.emit(':tell', "Sorry, " + outlet + " is not a valid outlet number.");
        }
    },
    'TurnOnOutlet': function () {
      let outlet = this.event.request.intent.slots.Outlet.value;
      if (outlet === undefined){
        doHTTP("on", "on", this, "Turning on both outlets.");
      }else if (parseInt(outlet) == 1){
        doHTTP("on", undefined, this, "Turning on outlet one.");
      }else if (parseInt(outlet) == 2){
        doHTTP(undefined, "on", this, "Turning on outlet two.");
      }else{
        this.emit(':tell', "Sorry, " + outlet + " is not a valid outlet number.");
      }
    },
    'TurnOffOutlet': function () {
      let outlet = this.event.request.intent.slots.Outlet.value;
      if (outlet === undefined){
        doHTTP("off", "off", this, "Turning off both outlets.");
      }else if (parseInt(outlet) == 1){
        doHTTP("off", undefined, this, "Turning off outlet one.");
      }else if (parseInt(outlet) == 2){
        doHTTP(undefined, "off", this, "Turning off outlet two.");
      }else{
        this.emit(':tell', "Sorry, " + outlet + " is not a valid outlet number.");
      }
    },
    'Status': function () {
        let outlet = this.event.request.intent.slots.Outlet.value;
        doStatus(this, outlet);
    },
    'AMAZON.HelpIntent': function () {
        var speechOutput = "You can say turn on outlet one or what is the status of my outlets... What can I help you with?";
        var reprompt = "What can I help you with?";
        this.emit(':ask', speechOutput, reprompt);
    },
    'AMAZON.CancelIntent': function () {
        this.emit(':tell', 'Goodbye from Moddable HQ!');
    },
    'AMAZON.StopIntent': function () {
        this.emit(':tell', 'Goodbye from Moddable HQ!');
    }
};
