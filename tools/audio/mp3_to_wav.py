#mp3_to_wav.py
import sys
from pathlib import Path
from pydub import AudioSegment

def mp3_to_wav(input_mp3, output_wav,
               sample_rate=22050,
               bit_depth=16,
               channels=1):
    audio = AudioSegment.from_mp3(input_mp3)

    audio = audio.set_frame_rate(sample_rate)
    audio = audio.set_channels(channels)
    audio = audio.set_sample_width(bit_depth // 8)

    audio.export(output_wav, format="wav")
    print(f"Converted: {input_mp3} → {output_wav}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python converter.py <file.mp3>")
        sys.exit(1)

    input_mp3 = Path(sys.argv[1])

    if not input_mp3.exists():
        print(f"Error: file '{input_mp3}' not found")
        sys.exit(1)

    output_wav = input_mp3.with_suffix(".wav")

    mp3_to_wav(input_mp3, output_wav)