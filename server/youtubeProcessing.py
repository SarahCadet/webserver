#!/usr/bin/python3

#using this API and guide: https://pypi.org/project/youtube-transcript-api/

import sys
import json
from youtube_transcript_api import YouTubeTranscriptApi
from youtube_transcript_api.formatters import JSONFormatter
from youtube_transcript_api._errors import TranscriptsDisabled, VideoUnavailable, NoTranscriptAvailable

"""
https://www.youtube.com/watch?v=ApXoWvfEYVU&list=RDKp7eSUU9oy8&index=13
"""
def parse_link(link):
    parts = link.split('?')
    vstring = parts[1]
    vstring = vstring.split('&')
    id = vstring[0]
    id = id[2:]
    return id

#assuming called like
#python3 webscraper.py https://www.youtube.com/watch?v=ApXoWvfEYVU&list=RDKp7eSUU9oy8&index=13

def main():
    video_id = parse_link(sys.argv[1])
    try:
        transcript_list = YouTubeTranscriptApi.list_transcripts(video_id)
        transcript = transcript_list.find_transcript(['en'])
    except (TranscriptsDisabled, VideoUnavailable, NoTranscriptAvailable) as e:
        print("<h1>Cannot fetch transcript.</h1>")
        sys.exit(1)
    if transcript:
        data = [{'start': i['start'], 'duration': i['duration'], 'subtitle': i['text']} for i in transcript.fetch()]
        with open("subtitles.txt", 'w') as f:
            for i in data:
                start = i['start']
                duration = i['duration']
                subtitle = i['subtitle']
                line = f"{start} {duration} {subtitle}\n"
                f.write(line)
    else:
        print("<h1>No english script.</h1>")
source = sys.argv[1].replace("watch?v=", "embed/")

print('<body style="text-align: center;"><iframe style="height: 460px; width: 700px;" src="' + source
      + '" allowfullscreen>Video Not Found</iframe>' + 
      '<form action="/playVideo"><input style="margin: 10px; border: 1px solid grey; border-radius: 40px; padding: 10px 15px;" type="submit" value="Play Video">' + 
      '</form></body>')

if __name__ == '__main__':
    main()