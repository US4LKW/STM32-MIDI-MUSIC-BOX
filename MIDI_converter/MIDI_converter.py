import mido
import math

def M2F(x):
    return 440.0 * pow(2.0, (x - 69.0) / 12.0)

def midi_to_custom_array(file_path):
    # Reading the MIDI file
    mid = mido.MidiFile(file_path)
    custom_array = [0xA000]  # Start of playback

    # Pre-computing the frequencies of the notes
    notes = [M2F(x) for x in range(127)]

    qnote = 0x75300
    time2 = 0

    for msg in mid:
        if not msg.is_meta:
            # Multiplying time by 2 to slow down playback
            time = int(mido.second2tick(msg.time, mid.ticks_per_beat, 500000) / mid.ticks_per_beat * 1000 * 2)
            time2 += time

            if msg.type == 'note_on' and msg.velocity > 0:
                while time2:
                    if time2 >= 0x8000:
                        custom_array.append(0xFFFF)
                        time2 -= 0x7FFF
                    else:
                        custom_array.append(0x8000 + time2)
                        time2 = 0

                tmp32 = int((notes[msg.note] * 4096.0 * 22050.0) / (notes[72] * 62500.0))
                if tmp32 < 0x8000:
                    custom_array.append(tmp32)
                else:
                    print(" ERROR_N ")

    custom_array.append(0xFFFF)  # End of playback
    return custom_array

file_path = 'MIDI.mid'  # Replace with the actual path to your file
custom_array = midi_to_custom_array(file_path)

# Convert values to hexadecimal with '0x' prefix and uppercase letters
hex_array = [f'0x{value:04X}' for value in custom_array]

# Print the list with 16 values per line
for i in range(0, len(hex_array), 16):
    print(', '.join(hex_array[i:i+16]))

# Save the string to a file in the desired format
output_file_path = 'music.h'
with open(output_file_path, 'w') as f:
    f.write('const uint16_t music[] = {\n')
    for i in range(0, len(hex_array), 16):
        f.write('    ' + ', '.join(hex_array[i:i+16]))
        if i + 16 < len(hex_array):
            f.write(',\n')
        else:
            f.write('\n')
    f.write('};\n')

output_file_path

# Keep the window open until the user closes it
input("Press Enter to exit...")
