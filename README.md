# plant-sensor

## Introduction

This project deals with the peripheral/mechanical side of a automatic plant waterer.
It is powered by a NRF52840 chip and communicates over bluetooth to a custom plant app, see: 'repo-ref'(TODO add link).

Sends live data about the plant to the app. Live data includes; temperature, humidity, water level in soil and sunlight.
Waters the plant when needed based on water level in soil measured.

sensors:
dht - measures temperature and humidity (considering bme280)

soil capacitive moisture sensor - measures moisture

TODO - add light sensor

TODO - add flow sensor (measure how much water has been used)

## Schematic

TODO - add link to schematic