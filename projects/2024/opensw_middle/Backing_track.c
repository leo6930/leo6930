#include <stdio.h>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

int main() {
    int choice;

    // 메뉴 구현
    printf("백킹트랙 목록:\n");
    printf("1: C major\n");
    printf("2: C# major\n");
    printf("3: D major\n");
    printf("4: Eb major\n");
    printf("5: E major\n");
    printf("6: F major\n");
    printf("7: F# major\n");
    printf("8: G major\n");
    printf("9: Ab major\n");
    printf("10: A major\n");
    printf("11: Bb major\n");
    printf("12: B major\n");
    printf("1번부터 12번까지 듣고싶은 트랙의 번호를 입력하세요 (1-12): ");
    scanf_s("%d", &choice);
    getchar();


    switch (choice) {
    case 1://C 
        PlaySound(TEXT("C.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        break;
    case 2: //C#
        PlaySound(TEXT("Jamtracks - Rock Clássico C#.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        break;
    case 3://D
        PlaySound(TEXT("D.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        break;
    case 4: //D#
        PlaySound(TEXT("Eb.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        break;
    case 5: //E
        PlaySound(TEXT("E.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        break;
    case 6: //F
        PlaySound(TEXT("F.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        break;
    case 7: //F#
        PlaySound(TEXT("F#.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        break;
    case 8: //G
        PlaySound(TEXT("G.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        break;
    case 9: //Ab
        PlaySound(TEXT("Ab.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        break;
    case 10: //A
        PlaySound(TEXT("A.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        break;
    case 11: //Bb
        PlaySound(TEXT("Bb.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        break;
    case 12: //B
        PlaySound(TEXT("B.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        break;
    default:
        printf("올바르지 않은 입력입니다.\n");
        break;
    }

    if (choice >= 1 && choice <= 12) {
        printf("음악 재생을 멈추려면 아무키나 누르세요.\n");
        getchar();
        PlaySound(NULL, NULL, SND_PURGE); //음악 멈추는 코드
    }

    return 0;
}
