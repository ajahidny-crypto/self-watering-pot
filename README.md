# Self-Watering Plant Pot

A 3D-printed plant pot that waters itself and keeps the wiring out of sight.

DIY self-watering setups usually leave a breadboard, a tangle of jumper wires and a pump sitting next to the plant. This pot keeps all of that in a snap-shut compartment underneath, so from the outside it just looks like a pot. A small pump fills a hollow channel around the rim, and water dribbles onto the soil through a ring of small drip holes.

<!-- TODO: add a photo or render of the finished pot here -->

## How it works

1. A soil moisture sensor checks the soil every 30 minutes.
2. When the soil is dry, the pump pulls water from a reservoir (a cup of water works fine) and pushes it through the 8.15 mm inlet on the outside of the pot.
3. The water fills the hollow channel inside the rim.
4. It dribbles out through the 2.5 mm drip holes in the inner wall, so it spreads around the plant instead of soaking one spot.
5. The pump waters in short pulses and checks the soil again until it's moist enough.

### Why it waters in pulses

The channel holds only about 30 mL, and the pump fills it much faster than 2.5 mm holes can drain it. If the pump ran until the sensor read wet, the channel would overflow and dump water on one side, and the pot would be flooded by the time the water reached the sensor.

The code fills the channel once, waits for it to drain, and repeats a couple of times. Then it waits for the water to soak down to the sensor before deciding whether to water again. This waters evenly and keeps it from overshooting.

## Dimensions

### Pot

| Feature | Size |
|---|---|
| Diameter | 123 mm |
| Depth | 112 mm |
| Outer wall | 4 mm |
| Hollow channel width | ~7 mm |
| Hollow channel depth | 13 mm |
| Inner (drip) wall | 2 mm |
| Drip holes | 2.5 mm diameter |
| Pump inlet | 8.15 mm diameter |
| Channel volume | ~30 mL |

### Electronics compartment (underneath)

| Feature | Size |
|---|---|
| Diameter | 120 mm |
| Depth | 50 mm |
| Cable/tube ports | 2, on opposite sides, 9 × 14 mm, about 12-15 mm through |
| Lid | Snap-fit |

The USB power cable goes through one port, and the pump tube and wire go through the other.

## Parts

| Part | Qty | Notes |
|---|---|---|
| Printed pot body | 1 | |
| Printed electronics compartment and lid | 1 | |
| Arduino Nano | 1 | Any 5 V ATmega328P board works |
| Capacitive soil moisture sensor (v1.2 / v2.0) | 1 | The bare-metal resistive kind corrodes in wet soil |
| 3-5 V mini submersible water pump | 1 | Sits in the reservoir cup |
| Silicone tubing | ~30 cm | Sized to fit the pump outlet and the 8.15 mm inlet |
| Logic-level N-MOSFET (e.g. IRLZ44N) | 1 | Or a ready-made MOSFET switch module |
| Diode (1N5819 or 1N4007) | 1 | Flyback protection for the pump |
| 220 Ω resistor | 1 | Gate resistor |
| 10 kΩ resistor | 1 | Gate pull-down |
| USB cable and 5 V phone charger | 1 | Power |
| Cup or jar | 1 | Water reservoir |

## Printing

- Supports: none
- Infill: 10%
- Material: PETG is a good choice for anything that holds water <!-- TODO: confirm what you printed with -->
- Use at least 3-4 perimeters so the channel walls are watertight. Those walls are all perimeters, so 10% infill doesn't affect them.

## Wiring

```
Pump (+) ──────────────── 5V
Pump (−) ──────────────── MOSFET Drain
MOSFET Source ─────────── GND
MOSFET Gate ── 220Ω ───── D3
MOSFET Gate ── 10kΩ ───── GND        (keeps the pump off while the board boots)
Diode across the pump    (striped end to Pump +)

Sensor VCC ────────────── D7         (powered only while reading)
Sensor GND ────────────── GND
Sensor AOUT ───────────── A0
```

## Code

The sketch is `self_watering_pot.ino`. To upload it:

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Put `self_watering_pot.ino` in a folder called `self_watering_pot`.
3. Open it, select **Tools → Board → Arduino Nano** and pick the right port.
   Cheap clone Nanos may need **Processor → ATmega328P (Old Bootloader)**.
4. Click **Upload**. You don't need any extra libraries.

### Calibrating

You only need to do this once. Open **Tools → Serial Monitor** and set it to 9600 baud and Newline.

1. Hold the sensor in the air and send `r`. Put the raw number in `DRY_RAW`.
2. Stand the sensor in a glass of water up to its line and send `r`. Put the raw number in `WET_RAW`.
3. With the tube connected and the pump in the cup, send `t` to start the pump. Press Enter the moment the channel is full, and put the printed time in `FILL_MS`.
4. Send `p` to fill the channel once and time how long it takes to empty. Put that time, plus a few seconds, in `DRAIN_MS`.
5. Upload the sketch again.

### Settings

These are at the top of the sketch:

| Setting | Default | What it does |
|---|---|---|
| `DRY_PERCENT` | 35 | Start watering below this |
| `WET_PERCENT` | 55 | Stop once it reaches this |
| `CHECK_INTERVAL_MS` | 30 min | How often it checks the soil |
| `FILL_MS` | 1500 ms | Pump time to fill the channel once |
| `PUMP_POWER` | 255 | Pump speed (0-255). Lower fills more gently, but some pumps stall below ~150 |
| `DRAIN_MS` | 30 s | Wait between pulses |
| `PULSES_PER_ROUND` | 2 | Fills per round (~60 mL) |
| `SOAK_MS` | 10 min | Wait for water to reach the sensor before checking again |
| `MAX_PULSES_PER_DAY` | 12 | Hard limit (~360 mL/day) |

Plants like different amounts of water, so adjust `DRY_PERCENT` and `WET_PERCENT` for yours.

### Serial commands

| Command | Action |
|---|---|
| `r` | Read the sensor |
| `t` | Timing mode for calibrating `FILL_MS` |
| `p` | Run one pulse |
| `w` | Start a watering round now |
| `c` | Clear the error state |
| `s` | Show status |

### Safety features

The sketch never runs more than `MAX_PULSES_PER_DAY` pulses in 24 hours, so a faulty sensor can't empty the cup onto your table.

If three watering rounds in a row don't raise the moisture reading, it assumes the cup is empty or the tube came loose. It stops pumping and fast-blinks the onboard LED. Refill the cup, then send `c` or press the reset button.

## Assembly

1. Print the pot and the electronics compartment.
2. Wire up the Nano, MOSFET and sensor (see Wiring) and fit them in the compartment.
3. Push the sensor into the soil and run its wire down through a port.
4. Put the pump in the cup, connect the tube to the 8.15 mm inlet, and run the pump wire through the other port.
5. Upload the code and calibrate.
6. Snap the compartment shut and plug in the USB cable.

## Tips

- Keep the cup's water level below the inlet hole. If the cup sits higher than the pot's rim, water can siphon through the pump and keep flowing after the pump stops.
- Test with the pot empty first to check for leaks around the inlet.
- If water only comes out of the drip holes nearest the inlet, check that the pot is level, or lower `PUMP_POWER` so the channel fills more evenly.
- Clean the drip holes now and then, since 2.5 mm holes can clog with mineral buildup. Filtered water or rainwater helps.

## License

TODO
