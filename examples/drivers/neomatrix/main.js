import parseBMP from 'commodetto/parseBMP'
import Poco from 'commodetto/Poco'
import Resource from 'Resource'
import Timer from 'timer'

const heart = parseBMP(new Resource('heart-alpha.bmp'))

const render = new Poco(global.screen, {
  pixels: 256
})

function getTick (heartRate, fps) {
  return heartRate * 6 / fps
}

const FPS = 20
const black = render.makeColor(0, 0, 0)

let heartRate = 60
let deg = 0

Timer.repeat(() => {
  deg = (deg + getTick(heartRate, FPS)) % 360
  const strength = 255 * Math.sin(Math.PI * deg / 360)
  const color = render.makeColor(strength, 0, 0)
  render.begin()
  render.fillRectangle(black, 0, 0, 16, 16)
  render.drawGray(heart, color, 0, 0)
  render.end()
}, 1000 / FPS)
