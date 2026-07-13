#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 
#include <windows.h> 
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")


// 스케일 구조체 정의
typedef struct {
    char* name;      // 스케일 이름
    char* notes[7];  // 스케일의 7개 음
} Scale;

//  Major 스케일
Scale scales[] = {
    {"C major", {"C", "D", "E", "F", "G", "A", "B"}},
    {"C# major", {"C#", "D#", "E#", "F#", "G#", "A#", "B#"}},
    {"D major", {"D", "E", "F#", "G", "A", "B", "C#"}},
    {"Eb major", {"Eb", "F", "G", "Ab", "Bb", "C", "D"}},
    {"E major", {"E", "F#", "G#", "A", "B", "C#", "D#"}},
    {"F major", {"F", "G", "A", "Bb", "C", "D", "E"}},
    {"F# major", {"F#", "G#", "A#", "B", "C#", "D#", "E#"}},
    {"G major", {"G", "A", "B", "C", "D", "E", "F#"}},
    {"Ab major", {"Ab", "Bb", "C", "Db", "Eb", "F", "G"}},
    {"A major", {"A", "B", "C#", "D", "E", "F#", "G#"}},
    {"Bb major", {"Bb", "C", "D", "Eb", "F", "G", "A"}},
    {"B major", {"B", "C#", "D#", "E", "F#", "G#", "A#"}}
};

// 펜타토닉 스케일을 동적 할당을 이용해서 생성
char** createPentatonic(Scale major) {
    char** pentatonic = malloc(5 * sizeof(char*));
    if (!pentatonic) {
        fprintf(stderr, "메모리 할당 실패\n");
        exit(EXIT_FAILURE);
    }
    pentatonic[0] = major.notes[0];
    pentatonic[1] = major.notes[1];
    pentatonic[2] = major.notes[2];
    pentatonic[3] = major.notes[4];
    pentatonic[4] = major.notes[5];
    return pentatonic;
}

// 펜타토닉 무작위 16개 음 출력
void printRandomNotes(char** scale, int count) {
    srand((unsigned)time(NULL));
    int previousIndex = rand() % 5;  // 초기 음 랜덤으로 선택
    int noteIndex = previousIndex;

    printf("%s ", scale[previousIndex]);

    for (int i = 1; i < count; i++) {
        do {
            noteIndex = rand() % 5;
        } while (noteIndex == previousIndex || abs(noteIndex - previousIndex) > 2);
        //이전 음과 거리가 2 초과 하지 않게
        printf("%s ", scale[noteIndex]);
        previousIndex = noteIndex;
    }
    printf("\n");
}

int main() {
    char input[100];

    while (1) {
        printf("어떤 스케일을 사용하고 싶습니까? (입력 종료는 'close'): ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0; //

        if (strcmp(input, "close") == 0) break;

        int found = 0;
        for (int i = 0; i < sizeof(scales) / sizeof(Scale); i++) {
            if (strcmp(scales[i].name, input) == 0) {
                found = 1;
                printf("%s 스케일: ", scales[i].name);
                for (int j = 0; j < 7; j++) {
                    printf("%s ", scales[i].notes[j]);
                }
                printf("\n");

                char** pentatonic = createPentatonic(scales[i]);
                printf("%s major pentatonic: ", scales[i].name);
                for (int k = 0; k < 5; k++) {
                    printf("%s ", pentatonic[k]);
                }
                printf("\n");

                printf("무작위 16개 음을 출력해드릴까요? (y/n): ");
                fgets(input, sizeof(input), stdin);
                input[strcspn(input, "\n")] = 0;
                if (strcmp(input, "y") == 0) {
                    printRandomNotes(pentatonic, 16);
                }

                free(pentatonic);
                break;
            }
        }
        if (!found) {
            printf("해당 스케일을 찾을 수 없습니다.\n");
        }
    }

    return 0;
}
