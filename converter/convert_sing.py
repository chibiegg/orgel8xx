# encoding=utf-8

import midi
from midi import NoteOnEvent, NoteOffEvent

def note2freq(note):
    return (440 * pow(2, float(note + (12 * 1)) / 12)) / 100

def freq2node(freq):
    return 69 + 12 * log(freq / 440) / log(2)


CHANNEL_NUM = 3


if __name__ == "__main__":
    import sys
    
    pattern = midi.read_midifile(sys.argv[1])

    ch = 0

    ch_freq = []
    notes = []

    for track in pattern:
        time = 0
        find_noteon = False
        note_events = []

        for event in track:
            time += event.tick
            if isinstance(event, (NoteOnEvent, NoteOffEvent)):
                event.tick = time
                note_events.append(event)
                find_noteon = True


    note_events.sort(key=lambda x:x.tick)

    for i, event in enumerate(note_events):
        freq = note2freq(event.data[0])
        velocity = event.data[1]
        
        if velocity:
            for ch in range(CHANNEL_NUM):
                if ch_freq[ch] == 0:
                    # 空きチャンネル発見
                    notes.append((event.tick, ch, freq, velocity))
        else:
            for ch in range(CHANNEL_NUM):
                if ch_freq[ch] == freq:
                    ch_freq[ch] = 0
                    # 停止
                    notes.append((event.tick, ch, 0, 0))


    for time, ch, freq, velocity in notes:
        print "{%d,\t%d,\t%d,\t%d}," % (time / 120, ch, freq, velocity)




