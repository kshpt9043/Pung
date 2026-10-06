# 최소 HUD 만들기 가이드

> C++ 부모 위젯 `UPungHUDWidget` 이 게임과의 연결(이벤트 바인딩, 리스폰 후 재연결)을 전부 해 준다.
> 블루프린트에서는 **배치하고, 이벤트를 구현하고, Get 함수로 값을 읽기만** 하면 된다.
> 이름(WBP_ 등)은 제안이다.

---

## 0. 준비

- [ ] `git pull` 후 `Build-Editor.bat` 으로 빌드 (UMG 모듈이 새로 들어갔다)
- [ ] 폴더 만들기: `Content/_Game/UI/`

---

## 1. 입력: 점수판 키

- [ ] `IA_Scoreboard` 만들기 (`Content/_Game/Input/Actions/`, Value Type = Digital (bool))
- [ ] `IMC_Default` 에 **Tab → IA_Scoreboard** 추가
- [ ] `BP_PungPlayerController` → UI 카테고리 → **Scoreboard Action = IA_Scoreboard**

점수판 키는 캐릭터가 아니라 컨트롤러가 받는다. 죽어서 관전 중일 때도 볼 수 있다.

---

## 2. HUD 위젯 만들기

- [ ] `WBP_PungHUD` 만들기: 콘텐츠 브라우저 우클릭 → User Interface → Widget Blueprint → 부모 클래스 선택에서 **`PungHUDWidget`**
- [ ] `BP_PungPlayerController` → UI → **HUD Widget Class = WBP_PungHUD**

이것만 해도 PIE 를 켜면 내 화면에만 HUD 가 뜬다 (아직 비어 있음).

### 추천 배치 (Designer 탭)

```
[Canvas Panel]
├─ Crosshair            (Image, 가운데 앵커, 8x8)
├─ TimerText            (Text, 위 가운데)                       "4:32"
├─ ChargeBox            (Horizontal Box, 아래 가운데)
│   ├─ ChargePip0~2     (Image 3개 또는 Progress Bar 3개)
│   └─ RechargeBar      (Progress Bar, 탄 칸 아래 얇게)
├─ InvulnerableText     (Text, 가운데 아래, 기본 숨김)           "무적 2.1"
├─ ItemBox              (Horizontal Box, 아래 오른쪽)
├─ KillFeedBox          (Vertical Box, 위 오른쪽)
├─ DeathPanel           (Overlay, 화면 가운데, 기본 숨김)
│   ├─ SpectateText     (Text)                                  "관전 중: Bot 2"
│   └─ RespawnText      (Text)                                  "3"
├─ ScoreboardPanel      (Border + Vertical Box, 가운데, 기본 숨김)
└─ ResultPanel          (Border + Text, 가운데, 기본 숨김)        "우승: kshpt9043"
```

변수로 쓸 위젯은 디테일 창에서 **Is Variable** 체크.

---

## 3. 요소별 연결 (Graph 탭)

이벤트는 Graph 의 My Blueprint → Functions → **Override** 목록에 `On ...` 으로 보인다.
매 프레임 바뀌는 값(시간, 게이지)은 **Event Tick** 에서 읽거나 텍스트의 **Bind** 로 연결한다.

### 3-1. 조준점
- 배치만 하면 끝.
- (선택) **On Fired** → 조준점을 잠깐 키웠다 줄이는 애니메이션 재생.

### 3-2. 매치 남은 시간
- TimerText → Text 옆 **Bind** → 새 함수에서 **Get Remaining Time Text** 반환. ("4:32" 형태로 나온다)

### 3-3. 탄과 충전 게이지
- **On Charges Changed (Charges, Max Charges)** → 칸 i 가 `i < Charges` 면 밝게, 아니면 어둡게.
- RechargeBar → Percent **Bind** → **Get Recharge Progress** (가득 차면 1).
- 죽어서 몸이 없으면 Charges = 0 으로 불린다.

### 3-4. 무적 표시
- InvulnerableText → Visibility **Bind**: **Is Invulnerable** 이면 Visible, 아니면 Collapsed.
- Text **Bind**: "무적 " + **Get Invulnerability Time Remaining** (소수 1자리).

### 3-5. 아이템
- **On Items Changed** → ItemBox **Clear Children** → **Get Pung Character → Items → Get Active Items** 를 돌며 아이콘 추가, **Get Held Items** 도 같은 방식 (사용형은 "F" 표시).
  - 아이템 정보: 항목의 Item → Icon, Color, Display Name
  - 지속형 남은 시간: Items → **Get Time Remaining (Item)** (Tick 이나 Bind 에서)
- 아이콘 하나짜리 작은 위젯(`WBP_ItemIcon`)을 따로 만들면 편하다.

### 3-6. 킬 피드
- 줄 하나짜리 위젯 `WBP_KillFeedEntry` 만들기 (Text 하나, 4초 뒤 Remove From Parent).
- **On Kill Feed (Killer, Victim)** →
  - Killer 가 None 이면: "[Victim 이름] 추락" (자멸)
  - 아니면: "[Killer 이름] → [Victim 이름]"
  - 이름: **Get Player Name Text (Player State)**
  - 내가 관련된 줄은 강조: **Is Local Player (Player State)**
  - Create Widget → KillFeedBox 에 **Add Child**. 5줄 넘으면 맨 위 줄 제거.

### 3-7. 사망 화면 (관전 + 리스폰 카운트다운)
- **On Spectate Changed (Spectating, Target)** →
  - Spectating 이면 DeathPanel 보이기, 아니면 숨기기
  - Target 이 있으면 SpectateText = "관전 중: " + 이름, None 이면 "자멸" 또는 빈칸
- RespawnText → Text **Bind**: **Get Respawn Time Remaining** 을 **Ceil** 해서 "3", "2", "1"

### 3-8. 점수판 (Tab)
- 줄 하나짜리 위젯 `WBP_ScoreboardRow` 만들기 (이름, 킬, 사망 Text 3개).
- **On Scoreboard Toggled (Visible)** → ScoreboardPanel 보이기/숨기기.
- **On Scoreboard Changed** → ScoreboardPanel 의 Vertical Box **Clear Children** →
  **Get Pung Game State → Get Sorted Players** 를 돌며 줄 추가.
  - 줄 내용: Player State → **Get Player Name**, **Get Kills**, **Get Deaths**
  - 봇 표시: **Is Bot** 이면 이름 앞에 "[BOT]" 같은 표시
  - 내 줄 강조: **Is Local Player**

### 3-9. 매치 결과 화면
- **On Match Phase Changed (New Phase)** →
  - **Ended**: ResultPanel 보이기 → **Get Pung Game State → Get Winners** 이름들을 이어 붙여 "우승: ..." (여러 명이면 공동 우승). 점수판도 같이 보여 주면 좋다.
  - **In Progress**: ResultPanel 숨기기
- 결과 화면 10초 뒤 맵이 다시 열리며 HUD 도 새로 만들어진다.

---

## 4. 이벤트 / 함수 요약

| 이벤트 (Override) | 언제 |
|---|---|
| On Pawn Changed (New Character) | 스폰, 리스폰, 죽어서 몸이 사라짐(None) |
| On Charges Changed (Charges, Max) | 탄 수 변화. 몸이 바뀌면 한 번 |
| On Fired | 내가 쐈을 때 |
| On Items Changed | 아이템 획득, 만료, 사용. 몸이 바뀌면 한 번 |
| On Kill Feed (Killer, Victim) | 누군가 떨어짐. Killer None = 자멸 |
| On Match Phase Changed (New Phase) | 매치 단계 변화. 처음 연결될 때 한 번 |
| On Spectate Changed (Spectating, Target) | 사망 후 관전 시작/끝 |
| On Scoreboard Changed | 입장, 퇴장, 킬, 사망, 이름 변경. 처음 연결될 때 한 번 |
| On Scoreboard Toggled (Visible) | Tab 누름/뗌 |

| 함수 (Get) | 값 |
|---|---|
| Get Remaining Time / Get Remaining Time Text | 매치 남은 시간 / "4:32" |
| Get Charges / Get Max Charges / Get Recharge Progress | 탄 수 / 최대 / 다음 탄 진행률 0~1 |
| Is Invulnerable / Get Invulnerability Time Remaining | 무적 여부 / 남은 초 |
| Is Waiting To Respawn / Get Respawn Time Remaining | 리스폰 대기 여부 / 남은 초 |
| Get Kills / Get Deaths | 내 기록 |
| Is Scoreboard Visible | Tab 누르는 중인지 |
| Get Pung Character / Get Pung Player State / Get Pung Game State | 더 필요한 값은 여기서 직접 |
| Get Player Name Text (Player State) | 이름. None 이면 빈 텍스트 |
| Is Local Player (Player State) | 나인지 |

---

## 5. 확인

- [ ] PIE 혼자: 시간이 줄어드는지, 쏘면 탄 칸이 줄고 게이지가 차는지
- [ ] `PungGiveItem DA_Item_Drum` → 아이템 아이콘, 남은 시간
- [ ] `PungAddBot 1` → 봇에게 떨어지면 킬 피드, 관전 패널, 3·2·1 카운트다운, 리스폰 후 HUD 가 새 몸으로 다시 동작 (탄 칸 갱신)
- [ ] Tab → 점수판 (봇 표시, 내 줄 강조)
- [ ] `BP_PungGameMode` Match Duration 을 잠깐 30 으로 → 결과 화면 → 10초 뒤 새 매치
- [ ] PIE 2인: 각 창에 자기 HUD 만 뜨는지 (호스트 화면에 HUD 가 두 개 겹치면 Claude 에게)
