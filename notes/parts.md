# Board

ESP-32 board - https://www.amazon.com/dp/B0D8T53CQ5.

I bought three of these for $20 already, without doing much research. I'm going to
default to using this, but it's possible I'll need to switch to something else.

- 0.5MB memory...
- Bluetooth/Wifi
- Supports Rust `std` library builds, which makes things easier.

## Links

- [Rust firmware](https://docs.espressif.com/projects/rust/book/overview/using-the-standard-library.html)
- [Chip data sheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
- [Full reference manual](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
(way too complicated for me)

# Display

Decisions

- Going LED because e-paper costs $$.
- Are we going battery powered or not?
  - Need battery life estimates for different variations of LEDs, batteries.
  - If we go no battery then we can have a larger, higher-res display.
- Better to show multiple rows of departure in a more square-ish display, or one row
on a long skinny one?

Options

- 96px x 48px (9.44in x 4.72in) [Amazon](https://www.amazon.com/2048-Matrix-Adjustable-Brightness-Compatible/dp/B0BRBDNT4L/ref=sr_1_1?crid=25EGMLJJRPUUJ&dib=eyJ2IjoiMSJ9.U8JAtGp6TvIOO6V26KArVDr-s1yFqEXt77-2HYR_1yVGe1IrdKoEq8bhH3qYgBlb6_yqEDFrl7gqeYsmQ0UcL3dhHNg5MeKSKlE7ce2sx0RPfcIrRs1N5qPMxACuR4JyTf6t4GSwcfiQyvYQhYDvCg.lJfzF_J2aK_0vcY-CeLb3Z6W-oX00_q5PpNaF0PbIYA&dib_tag=se&keywords=%E2%80%9CHUB75%2BP2.0%2B128%C3%9764%2BRGB%2BLED%2Bmatrix%2B256%C3%97128%2Bmm%E2%80%9D&qid=1758402267&s=electronics&sprefix=hub75%2Bp2.0%2B128%2B64%2Brgb%2Bled%2Bmatrix%2B256%2B128%2Bmm%2B%2Celectronics%2C89&sr=1-1&th=1)
  - The amazon pictures show 5 rows of large-ish text.
  - Slightly bendy, could be mounted on a curved surface.

# Battery

My understanding is you can just use any old USB-C power bank.

But I wonder if there are more OEM-style ones that will be cheaper and easier to mount
with the input charging cable outside of the case and the output cable inside to the
board?

I'm down to get a fat battery to make this thing feel heavy and high quality too.

I need to come up with a model for power draw to decide if / which battery to buy.
