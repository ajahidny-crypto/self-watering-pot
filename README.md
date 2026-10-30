# Hidden Drip Pot

A 3D-printed self-watering plant pot that hides all the electronics underneath it.

A small pump fills a hollow ring around the top of the pot, and the water drips onto the soil through small holes. A moisture sensor in the soil decides when to water. The Arduino and all the wiring sit in a compartment under the pot, so from the outside it just looks like a normal pot.

![Isometric view of the pot and the base lid](02_isometric_view.png)

![Close-up of the rim, showing the hollow ring and drip holes](01_rim_drip_holes_closeup.png)

## Why I made it

I had two problems with self-watering setups. The first was how messy the circuitry gets. A breadboard and a bunch of wires look ugly next to a flower pot, so I hid all of it under the pot in a snap-shut compartment.

The second was the code. Getting the pump to fill that small hollow ring properly while also working with a moisture sensor took some figuring out.

## How it works

1. The sensor checks the soil every 30 minutes.
2. If the soil is dry, the pump pushes water from a cup into the big hole on the side of the pot.
3. The water fills the hollow ring around the top.
4. It drips out through the small holes and spreads around the plant instead of soaking one spot.

![Front view showing the pump inlet near the top and a cable port at the bottom](06_front_view_inlet_and_port.png)

## Design

| Part | Size |
|---|---|
| Pot diameter | 123 mm |
| Pot depth | 112 mm |
| Total height | about 165 mm |
| Outer wall | 4 mm |
| Hollow ring | about 7 mm wide, 13 mm deep |
| Inner wall | 2 mm |
| Drip holes | 2.5 mm |
| Pump inlet | 8.15 mm |
| Electronics compartment | 120 mm wide, 50 mm deep |
| Cable ports | 2, on opposite sides, 9 × 14 mm |

I printed it with no supports and 10% infill.

![Side view showing the total height](03_side_view_height.png)

![Bottom view of the electronics compartment and its lid](04_bottom_view_compartment_and_lid.png)

![Top view of the pot next to the lid](05_top_view.png)

## Getting the watering right

The ring only holds about 30 mL, and the pump fills it way faster than the little holes can drain it. If I just ran the pump until the soil read wet, the ring would overflow and flood one side of the pot.

So instead it waters in short bursts. It fills the ring, waits for it to drain, and does that twice. Then it waits 10 minutes for the water to soak down to the sensor and checks again. If the soil's still dry, it goes again.

I also added a couple of safety things:

- It won't pump more than 12 times in 24 hours (counted from when you plug it in), so a broken sensor can't empty the cup all over the table.
- If the soil isn't getting any wetter after a few rounds, it figures the cup is empty, stops, and blinks the light on the Arduino. Refill it and type `c` to start it back up.

## Parts

| Qty | Part |
|---|---|
| 1 | Arduino Nano Every (ABX00028) |
| 1 | DFRobot Gravity: Analog Capacitive Soil Moisture Sensor (SEN0193) |
| 1 | DFRobot Gravity: Analog Sensor Cable for Arduino, 50 cm |
| 1 | Adafruit Submersible 3V DC Water Pump, Vertical Type (4547) |
| 1 | Adafruit Tubing for Submersible Pumps, PVC 6mm ID (4545) |
| 1 | DFRobot Gravity: MOSFET Power Controller (DFR0457) |
| 1 | 1N5819 diode |
| 1 | Micro-USB cable and a phone charger |
| 1 | Cup of water |

Everything fits in the compartment with room to spare. The tubing is 8 mm on the outside, so it fits snugly into the 8.15 mm inlet. The pump sits in the cup.

## Wiring

```
Pump module + and −  ──  Arduino 5V and GND
Pump module output   ──  pump red and black wires
Pump module signal   ──  D3
Diode                ──  across the pump wires (striped end on red)

Sensor signal  ──  A0
Sensor power   ──  D7
Sensor ground  ──  GND
```

The USB cable goes through one port, and the pump wire, tube and sensor cable go through the other.

## Code

The code is in `self_watering_pot.ino`. To upload it, install the Arduino IDE, add "Arduino megaAVR Boards" in the Boards Manager, select Arduino Nano Every, and hit Upload.

Before using it, you need to set a few numbers at the top of the code. Open the Serial Monitor at 9600 baud and:

1. Hold the sensor in the air, type `r`, and put the number in `dryRaw`.
2. Put the sensor in a cup of water, type `r`, and put the number in `wetRaw`.
3. Type `t` to start the pump, press Enter as soon as the ring is full, and put the time it prints into `fillTime`.

Then upload it again. If your plant likes more or less water, change `dryPct` and `wetPct`.

Other commands: `p` runs the pump once, `w` waters right now, `s` shows the status, and `c` clears an error.

## Putting it together

1. Print the pot and the lid.
2. Wire everything up and put it in the compartment.
3. Push the sensor into the soil and run its cable down through a port.
4. Put the pump in the cup and push the tube into the inlet.
5. Upload the code and set it up (see above).
6. Snap the lid on and plug it in.

Keep the cup lower than the top of the pot. If it's higher, water can keep flowing even after the pump turns off.

## Files

| File | Description |
|---|---|
| `self_watering_pot.ino` | Arduino code |
| `01_rim_drip_holes_closeup.png` | Close-up of the ring and drip holes |
| `02_isometric_view.png` | Isometric view |
| `03_side_view_height.png` | Side view |
| `04_bottom_view_compartment_and_lid.png` | Compartment and lid |
| `05_top_view.png` | Top view |
| `06_front_view_inlet_and_port.png` | Front view with the inlet and port |
| `LICENSE` | Project license |

<!-- TODO: add the CAD and STL files here -->

## Tools and materials

- <!-- TODO: CAD software -->
- <!-- TODO: printer and filament -->
- Arduino IDE

## Future plans

- Find a better water container. A cup works for now.

## License

This project is open source. See LICENSE for the terms.
