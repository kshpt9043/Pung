# 에디터 작업 체크리스트

> 원격(클라우드)에서 C++ 로 만든 기능을 에디터에서 연결하고 확인하기 위한 목록.
> 위에서부터 순서대로 하면 된다. 앞 단계가 뒤 단계의 전제다.
> 이름(BP_, DA_, IA_ 등)은 제안이다. 다르게 지어도 되지만 지정하는 곳은 빠뜨리지 말 것.
> 작성: 2026-10-06 / 기준 브랜치: `claude/lucid-shannon-g7r55o`

---

## 0. 코드 받기와 빌드

- [ ] 에디터를 끈 상태에서 브랜치 받기
  ```
  git fetch origin
  git checkout claude/lucid-shannon-g7r55o
  git pull
  ```
  - `main` 에는 PR #1 (관전 카메라까지) 만 들어가 있다. **UI 데이터, 아이템은 이 브랜치에만 있다.**
- [ ] `Build-Editor.bat` 실행 → `===== 빌드 성공 =====` 확인
  - 엔진이 `C:\Program Files\Epic Games\UE_5.8` 이나 `E:\UE_5.8` 이 아니면 환경 변수 `UE_ENGINE_DIR` 을 엔진 폴더로 지정
  - **UI 데이터와 아이템 코드는 아직 한 번도 빌드되지 않았다.** 실패하면 로그 전체를 Claude 에게 붙여줄 것
  - 특히 볼 곳: 아이템 이름으로 찾는 Asset Registry (`PungGiveItem`), `GetTimerRemaining`, 아이템 효과 클래스의 한글 DisplayName
- [ ] 에디터 열기. "모듈을 다시 빌드할까요?" 가 나오면 아니오 (이미 빌드함). 컴파일 오류 창이 뜨면 로그를 Claude 에게

---

## 1. 게임 모드와 맵 기본 연결

지금 `Config/DefaultEngine.ini` 의 기본 게임 모드는 **템플릿의 `BP_FirstPersonGameMode`** 다. 이게 바뀌지 않으면 Pung 기능이 하나도 동작하지 않는다.

- [ ] 둘 중 하나 (둘 다 해도 됨)
  - [ ] Project Settings → Maps & Modes → **Default GameMode** = `BP_PungGameMode`
  - [ ] `Lvl_FirstPerson` 의 World Settings → **GameMode Override** = `BP_PungGameMode`
- [ ] `BP_PungGameMode` 의 Classes 확인
  - [ ] Default Pawn Class = `BP_PungCharacter`
  - [ ] Player Controller Class = `BP_PungPlayerController` (C++ 기본값은 `APungPlayerController` 라 입력 매핑이 안 붙는다)
  - [ ] Game State Class = `PungGameState` (C++ 기본값. 다른 걸로 바뀌어 있으면 되돌리기)
  - [ ] Player State Class = `PungPlayerState` (위와 같음)
- [ ] 확인: PIE 에서 `~` 콘솔 → `PungSession` 입력 → 화면 왼쪽 위에 `[세션] ...` 이 나오면 연결 성공 ("Command not recognized" 면 위 설정이 안 된 것)

---

## 2. 캐릭터 (`BP_PungCharacter`)

- [ ] 부모 클래스가 `PungCharacter` 인지 확인 (Class Settings → Parent Class)
- [ ] BP 를 열어 **Compile → Save** (C++ 에서 이동 컴포넌트 클래스와 아이템 컴포넌트가 바뀌었다)
- [ ] Components 패널
  - [ ] Character Movement 의 클래스가 `PungCharacterMovementComponent` 인지
  - [ ] Character Movement 값이 C++ 초기값과 같은지 (BP 에 예전 값이 남아 있을 수 있음)
    - Air Control = 0.3
    - Braking Deceleration Falling = 0
    - Gravity Scale = 2 (C++ 기본값. **맵 World Settings 의 Override World Gravity 는 꺼야 한다.** 켜 두면 중력이 겹쳐 4배가 된다)
    - Jump Z Velocity = 원래 값 × 1.41 (중력 2배에 맞춘 값)
    - **Nav Movement → Use Acceleration for Paths = 켬.** 끄면 봇이 가속도 없이 미끄러져 애님 BP 가 걷기로 인식하지 않는다
  - [ ] `Air Gun`, `Items` 컴포넌트가 보이는지
  - [ ] First Person Mesh (팔) 에 스켈레탈 메시와 애님 BP
  - [ ] Mesh (3인칭 몸, 다른 사람에게 보임) 에 스켈레탈 메시와 애님 BP
- [ ] Input 카테고리 (6개 전부)
  - [ ] Move Action = `IA_Move`
  - [ ] Look Action = `IA_Look`
  - [ ] Mouse Look Action = `IA_MouseLook`
  - [ ] Jump Action = `IA_Jump`
  - [ ] Fire Action = `IA_Fire`
  - [ ] **Use Item Action = `IA_UseItem`** (새로 만들어야 함, §3)
- [ ] Knockback 카테고리 (기본값 그대로 두고 테스트 후 조정)
  - Self Knockback Overrides Last Attacker = false (GDD §5.2 테스트 항목)
  - Knockback Control Duration = 0.45 / Knockback Control Scale = 0.1
  - Blast Jump Grace Time = 0.22
- [ ] Air Gun 컴포넌트 디테일
  - [ ] **Gun Data** = `DA_AirGunData` (§4 에서 만듦. 비우면 C++ 기본값으로 동작하지만, PIE 중 튜닝을 하려면 에셋이 있어야 함)
  - [ ] **Projectile Class** = `BP_PungAirProjectile` (§4. 비우면 탄이 안 보임)
  - Muzzle Offset = 60, Max Eye Location Error = 200 (그대로)
- [ ] Event Graph
  - [ ] **On Invulnerability Changed** → 무적 이펙트 켜고 끄기. **GDD §5.4: 다른 플레이어가 무적임을 볼 수 있어야 함.** 3인칭 몸(Mesh)에 머티리얼 파라미터나 나이아가라 등
  - [ ] (선택) **On Knocked Back** → 맞았을 때 이펙트, 사운드, 카메라 흔들림 (서버와 본인 화면에서 실행)
  - [ ] (선택) Air Gun 의 **On Fired** 바인딩 → 쏠 때 반동, 사운드 (본인 화면)
  - [ ] (선택) Items 의 **On Item Used** 바인딩 → 펄스 이펙트, 사운드 (모든 화면)

---

## 3. 입력

- [ ] `IA_UseItem` 만들기 (`Content/_Game/Input/Actions/`, Value Type = Digital (bool))
- [ ] `IMC_Default` 매핑 확인
  - [ ] IA_Move (WASD), IA_Jump (Space), IA_Look / IA_MouseLook (마우스) — 템플릿 그대로인지
  - [ ] **IA_Fire → 마우스 왼쪽** (이미 있는지 확인)
  - [ ] **IA_UseItem → F** (원작과 같음)
  - [ ] (UI 할 때) 점수판 키 IA_Scoreboard → Tab. C++ 에는 점수판 입력이 없으므로 BP_PungPlayerController 에서 처리
- [ ] `BP_PungPlayerController` → Default Mapping Contexts 에 `IMC_Default`, `IMC_MouseLook` 이 들어 있는지
- [ ] 확인: PIE 에서 걷기, 점프, 사격이 되는지

---

## 4. 무기

- [ ] `DA_AirGunData` 만들기 (Data Asset → `PungAirGunData`). 값은 GDD §10 그대로 두고 시작
  - Max Range 4000, Proximity Fuse Radius 130, Visual Projectile Speed 2500
  - Blast Radius 300, Knockback Strength 2120 (에디터 표시 21.2 m/s), Edge Strength Scale 0.25
  - Other Blast Radius Scale 1.6, Other Knockback Scale 0.82
  - Max Charges 3, Recharge Time 1.5, Fire Interval 0.12 (원작 연사 감각으로 변경)
- [ ] `BP_PungCharacter` → Air Gun → Gun Data 에 지정 (§2)
- [ ] `BP_PungAirProjectile` 만들기 (부모: `PungAirProjectile`)
  - [ ] 탄 외형 (작은 메시나 나이아가라) 을 Root 에 붙이기
  - [ ] **On Detonated (Location, Radius)** → 폭발 이펙트와 사운드를 Location 에 스폰. Radius 로 크기 맞추기 (탄 자체는 이 직후 사라진다)
- [ ] `BP_PungCharacter` → Air Gun → Projectile Class 에 지정 (§2)
- [ ] (선택) 화면 가운데 조준점 — UI 위젯에서 (§10)

---

## 5. 맵 / 테스트 아레나 (`Lvl_FirstPerson` 또는 새 맵)

- [ ] 회색 박스 아레나: LevelPrototyping 메시로, **떨어질 수 있는 가장자리**가 있게
- [ ] **Player Start 여러 개** (4~8개). 리스폰은 이 중 랜덤
- [ ] **Kill Z**: World Settings → Kill Z 를 아레나 바닥보다 적당히 아래로 (예: 바닥 -1000 cm). 기본값은 너무 낮아서 떨어진 뒤 한참 기다리게 된다. 또는 아레나 아래에 Kill Z Volume
- [ ] **NavMeshBoundsVolume** 을 아레나 전체를 덮게 배치 (봇 이동, 안전 지점 찾기). 뷰포트에서 `P` 키로 초록색 확인
- [ ] (선택) 관전 전경 위치: 아무 액터 (Camera Actor 추천) 를 아레나를 내려다보게 놓고 **Actor Tag 에 `PungOverview`** 추가. 없으면 Player Start 평균 위치 위에서 자동으로 내려다봄
- [ ] 아이템 패드 배치 (§6 에서 BP 를 만든 뒤)
- [ ] 맵이 바뀌면 Project Settings → Maps & Modes → Game Default Map / Editor Startup Map 도 바꾸기 (`Run-Standalone.bat` 은 Game Default Map 으로 시작)

---

## 6. 아이템

- [ ] 데이터 에셋 4개 만들기 (Data Asset → `PungItemData`)

  | 에셋 이름 (제안) | Kind | Duration / Charges | Effect (드롭다운) | 효과 수치 (기본값) |
  |---|---|---|---|---|
  | `DA_Item_Overcharge` | Timed | Duration 12 | 과충전 (폭발 강화) | Strength 1.8, Radius 1.25, Affect Self Blast 끔 |
  | `DA_Item_Anchor` | Timed | Duration 10 | 닻 (넉백 저항) | Knockback 0.35, Self Knockback 0.35 |
  | `DA_Item_Drum` | Timed | Duration 15 | 드럼 탄창 (빠른 충전) | Recharge Rate 3, Refill On Pickup 켬 |
  | `DA_Item_Pulse` | Charge | Charges 1 | 펄스 (주변 폭발) | Strength 2, Radius 1.6 |

  - [ ] 각 에셋에 Display Name, Description, Icon (텍스처), Color 도 채우기 (HUD, 패드 표식용)
  - [ ] **Effect 를 반드시 고를 것.** 비어 있으면 주워도 아무 효과가 없다
- [ ] `BP_PungGameMode` → Items → **Default Item Pool** 에 4개 넣기
- [ ] `BP_PungItemPad` 만들기 (부모: `PungItemPad`). C++ 패드는 범위(구)만 있고 **안 보인다**
  - [ ] 발판 메시 (원작: 캐릭터 지름 정도의 원판)
  - [ ] **On Item Changed (Item)** → 패드 위에 다음 아이템 표식 (Item 의 Icon/Color 사용). Item 이 None 이면 숨기기
  - [ ] **On Ready Changed (bNewReady)** → 비었을 때 표식 흐리게/숨기기, 찼을 때 보이기
  - [ ] **On Picked Up (Character, Item)** → 획득 사운드, 이펙트
  - [ ] (선택) 충전 링: Tick 또는 타이머에서 `Get Respawn Progress` (0~1) 로 링 채우기
  - 디테일: Item Pool (비우면 게임 모드 목록), Fixed Item (이것만 나옴), Respawn Time 20
- [ ] 맵에 `BP_PungItemPad` 2~4개 배치
- [ ] 확인
  - [ ] `PungGiveItem DA_Item_Pulse` → F 로 펄스 (이름이 틀리면 로그에 있는 아이템 목록이 나옴)
  - [ ] 패드 밟기 → 획득, 20초 뒤 다시 참, 다음 아이템 표식이 바로 바뀜
  - [ ] 드럼 탄창: 줍는 즉시 탄 가득, 충전이 빨라짐
  - [ ] 닻: 거의 안 밀림, 로켓 점프도 약해짐
  - [ ] 과충전: 남을 더 멀리 날림, 로켓 점프는 그대로

---

## 7. 봇

- [ ] `DA_BotProfile_Normal` 만들기 (Data Asset → `PungBotProfile`). 기본값으로 시작
  - (선택) `DA_BotProfile_Easy` (반응 0.6~1.0초, 오차 8°), `DA_BotProfile_Hard` (반응 0.15~0.3초, 오차 1.5°)
- [ ] 블랙보드 `BB_PungBot` 만들기. 키:
  - [ ] `TargetActor` — Object, Base Class = Actor
  - [ ] `bInDanger` — Bool
  - [ ] `MoveLocation` — Vector
- [ ] 비헤이비어 트리 `BT_PungBot` 만들기 (블랙보드 = `BB_PungBot`). 예시:
  ```
  Root
  └─ Selector  [Service: Pung Find Target (Target Key = TargetActor)]
               [Service: Pung Check Edge  (In Danger Key = bInDanger)]
     ├─ Sequence  [Decorator: Blackboard  bInDanger Is Set, Observer Aborts = Lower Priority]
     │   ├─ Pung Find Safe Location  (Location Key = MoveLocation)
     │   └─ Move To  (MoveLocation)
     ├─ Selector  [Decorator: Blackboard  TargetActor Is Set, Observer Aborts = Lower Priority]
     │   ├─ Pung Use Item          [Decorator: Is At Location (TargetActor, Acceptable Radius 600)]
     │   │                          (펄스는 가까울 때만. 없거나 멀면 실패하고 아래로)
     │   ├─ Simple Parallel        [Decorator: Pung Has Charge]   ← 쏘면서 움직이기
     │   │   ├─ (주 작업) Pung Aim And Fire  (Target Key = TargetActor)
     │   │   └─ (배경)   Sequence
     │   │                ├─ Pung Find Combat Location (Target Key = TargetActor, Location Key = CombatLocation)
     │   │                └─ Move To (CombatLocation, Acceptable Radius 100)
     │   │       * Simple Parallel 의 Finish Mode = Immediate (사격이 끝나면 이동도 끊고 다음 사격으로)
     │   └─ Sequence                                               ← 탄이 없을 때: 자리 잡으며 충전 대기
     │       ├─ Pung Find Combat Location (TargetActor → CombatLocation)
     │       ├─ Move To (CombatLocation)
     │       └─ Wait 0.3
     └─ Sequence   ← 배회 (적이 안 보일 때)
         ├─ Pung Find Roam Location  (Location Key = MoveLocation)
         ├─ Move To  (MoveLocation)
         └─ Wait 1.0 (Random Deviation 0.5)
  ```
  - [ ] **각 Pung 노드 디테일에서 블랙보드 키를 직접 골라야 한다** (C++ 에 이름을 박지 않았다)
  - [ ] `Pung Has Charge` 는 Observer Aborts 를 쓰지 말 것 (막아 둠)
  - [ ] `Pung Use Item` 은 상대가 가까울 때만 쓰게 엔진 기본 데코레이터 **Is At Location** (키 = TargetActor) 을 붙인다
  - [ ] 블랙보드에 **`CombatLocation` (Vector)** 키 추가 (배회용 MoveLocation 과 따로 두면 서로 덮어쓰지 않는다)
  - [ ] 모든 Move To 의 **Allow Partial Path 끄기** — 켜져 있으면 목적지까지 길이 없을 때 갈 수 있는 데까지 가서 벽 앞에 멈춘다. 끄면 실패하고 다음 판단으로 넘어간다
  - [ ] 전투용 Move To 의 **Allow Strafe 켜기** — 대상을 바라본 채 옆으로 움직인다 (조준은 Pung Aim And Fire 가 대상 쪽으로 고정)
  - [ ] TargetActor 데코레이터의 **Observer Aborts = Lower Priority** — 배회 중에 적이 보이면 바로 전투로 넘어간다
  - [ ] (선택) `Pung Find Target` 의 Target Airborne Key / Target Near Edge Key 에 Bool 키를 지정하면 "뜬 상대에게만 저글 가지" 같은 조건을 BT 에서 만들 수 있다. 비워 둬도 된다
- [ ] `BP_PungAIController` 만들기 (부모: `PungAIController`)
  - [ ] AI → **Behavior Tree** = `BT_PungBot`
  - [ ] AI → **Bot Profile** = `DA_BotProfile_Normal`
- [ ] `BP_PungGameMode` → Bots → **Bot Controller Class** = `BP_PungAIController` (비우면 BT 없는 C++ 기본 컨트롤러라 가만히 서 있음)
  - Max Players Without Session = 8 (세션 없이 넣을 수 있는 정원)
- [ ] 확인
  - [ ] `PungAddBot 1` → 봇이 적을 찾아 쏘는지, 가장자리에서 물러나는지, 패드에서 아이템을 줍는지
  - [ ] 봇을 떨어뜨리면 킬, 봇이 날 떨어뜨리면 봇 킬 (점수판 / 로그 `[낙사]`)
  - [ ] `PungRemoveBot 1`
  - [ ] 매치가 끝나고 다시 시작해도 봇 수가 유지되는지
  - [ ] Gameplay Debugger (`'` 키) 로 BT / 블랙보드 상태 확인 가능

### 7.1 똑똑한 봇 (등급 Smart, EQS)

자세한 조립 순서는 세션 가이드를 따른다. 빠뜨리지 말 것만 적는다.

- [ ] `DA_BotProfile_Smart` (Normal 복제 후: 반응 0.2~0.4초, 조준 지연 0.12초, 오차 2°, 저글 확률 0.8, Low Charge Target Bonus 700, Retaliation Target Bonus 500)
- [ ] (임시, 선택) 등급 구분 외형: 프로필 → Debug → **Body Look** 에 Material Override 또는 Use Tint + Body Tint + Tint Parameter Name
- [ ] 블랙보드 `BB_PungBot_Smart` (BB_PungBot 복제) 키 추가: `bThreatened` (Bool), `ThreatActor` (Object, Actor), `DodgeLocation` (Vector), `RetreatLocation` (Vector), `ItemLocation` (Vector)
- [ ] EQS 쿼리 3개: `EQS_Pung_Attack`, `EQS_Pung_Retreat`, `EQS_Pung_Item` (배회는 기존 `Pung Find Roam Location` 재사용)
- [ ] 비헤이비어 트리 `BT_PungBot_Smart` (블랙보드 = `BB_PungBot_Smart`)
  - [ ] 루트 아래 서비스: Pung Find Target, Pung Check Edge, **Pung Detect Threat** (Threatened Key = bThreatened, Threat Key = ThreatActor)
  - [ ] 모든 Run EQS Query 의 Run Mode = Single Best Item, Move To 의 Allow Partial Path 끄기
- [ ] `BP_PungAIController_Smart` (부모: `PungAIController`) → Behavior Tree = `BT_PungBot_Smart`, Bot Profile = `DA_BotProfile_Smart`
- [ ] `BP_PungGameMode` → Bots → **Bot Tiers** 에 항목 추가: 키 `Smart`, Controller Class = `BP_PungAIController_Smart`, Name Prefix = `Smart Bot`
- [ ] 확인
  - [ ] `PungAddBot 2 Smart` → 이름이 "Smart Bot N", 기본 봇과 섞어 넣기 (`PungAddBot 2`)
  - [ ] 내가 조준하고 있으면 옆으로 비켜서는지 (Gameplay Debugger 에서 bThreatened)
  - [ ] 탄이 1발 이하면 물러나는지, 가득 차면 다시 붙는지
  - [ ] `PungRemoveBot 1 Smart` → Smart 만 빠지는지. 매치 재시작 후 등급별 수 유지
  - [ ] Retreat/Item 쿼리는 EQS 테스트 폰(`EQSTestingPawn`)으로, Attack 쿼리는 PIE 중 Gameplay Debugger EQS 카테고리로 점수 확인 (Pung Target 은 봇 컨트롤러에서만 채워진다)

---

## 8. 관전 (리스폰 대기 중)

- [ ] 설정 없이 동작한다. 확인만:
  - [ ] 봇에게 밀려 떨어지면 그 봇을 3인칭으로 따라감
  - [ ] 혼자 떨어지면 (자멸) 아레나 전경
  - [ ] 리스폰하면 1인칭으로 돌아옴
  - [ ] 사망 순간 시점이 튀거나, 관전 중 1인칭으로 잠깐 돌아오는 현상이 있으면 Claude 에게 알리기
- [ ] (선택) 거리, 높이 조정: `BP_PungSpectatorCamera` (부모: `PungSpectatorCamera`) 만들어 `BP_PungPlayerController` → Spectate → **Spectator Camera Class** 에 지정. Spectate Blend Time 도 여기서

---

## 9. 디버그 도구 (테스트할 때 사용)

| 명령 | 용도 |
|---|---|
| `pung.Debug.Blast 1` | 조준선(흰), 자기 폭발 반경(노랑), 남 폭발 반경(청록), 넉백 방향, 벽에 막힌 대상(회색). **탄이 안 보일 때도 판정 확인 가능** |
| `pung.Knockback.ClientApply 0/1` | 넉백을 본인 화면에서도 바로 적용할지 (끊김 비교) |
| `pung.Debug.Trajectory 1` | 점프, 로켓 점프, 넉백 뒤 비행 궤적과 "시간, 최고 높이, 수평 거리" 표시. 넉백 튜닝과 맵 치수 측정용 |
| `PungAddBot [수] [등급]` / `PungRemoveBot [수] [등급]` | 봇 추가/제거 (호스트). 예: `PungAddBot 2 Smart` |
| `PungStartMatch` / `PungEndMatch` | 대기 중 바로 시작 / 진행 중 바로 종료 (호스트) |
| `PungGiveItem <에셋 이름>` | 아이템 받기 (호스트) |
| `PungHost [인원]` / `PungFind` / `PungJoin [번호]` / `PungLeave` / `PungInvite` / `PungSession` / `PungBuildFilter 0/1` | Steam 세션 |

---

## 10. UI (UserWidget) — 위젯이 쓸 데이터는 전부 준비되어 있다

전체 목록은 GDD §10.5 "UI 에서 쓸 데이터" 표. 만들 것:

> **최소 HUD 는 C++ 이 값을 채운다 (BindWidget).** WBP 에서 정해진 이름으로 위젯을 배치하기만 하면 된다. 필수 위젯이 없으면 WBP 컴파일 에러가 난다.
> - `WBP_PungHUD` (부모 `PungHUDWidget`): TimerText, ChargeBox, KillFeedBox, DeathPanel, ScoreboardPanel, ScoreboardList, ResultPanel (필수) / RechargeBar, InvulnerableText, ItemBox, SpectateText, RespawnText, ResultText, CountdownText (선택)
> - `WBP_KillFeedEntry` (부모 `PungKillFeedEntryWidget`): MessageText
> - `WBP_ScoreboardRow` (부모 `PungScoreboardRowWidget`): NameText, KillsText, DeathsText (필수) / RankText, LocalHighlight (선택)
> - `WBP_ItemSlot` (부모 `PungItemSlotWidget`): IconImage (필수) / InfoText, TimeBar (선택)
> - `WBP_PungHUD` Class Defaults 에서 Kill Feed Entry Class, Scoreboard Row Class, Item Slot Class 지정

- [ ] HUD 위젯을 화면에 붙이기: `WBP_PungHUD` (부모 `PungHUDWidget`) 를 만들어 `BP_PungPlayerController` → UI → **HUD Widget Class** 에 지정 (내 화면에만 자동 생성)
- [ ] 점수판 키: `IA_Scoreboard` (Tab) 를 `BP_PungPlayerController` → UI → **Scoreboard Action** 에 지정
- [ ] 조준점
- [ ] 매치 남은 시간 — GameState `Get Remaining Time`
- [ ] 탄 충전 수와 게이지 — Air Gun `Get Charges`, `Get Max Charges`, `On Charges Changed`, `Get Recharge Progress`
- [ ] 아이템 HUD — Items `Get Active Items` (+ `Get Time Remaining`), `Get Held Items`, `On Items Changed`. 사용형은 "F" 키 표시
- [ ] 무적 표시 — Character `Is Invulnerable`, `Get Invulnerability Time Remaining`
- [ ] 킬 피드 — GameState `On Player Fell (Killer, Victim)`. Killer 가 None 이면 자멸
- [ ] 사망 화면 — Controller `On Spectate Changed (관전 중, 대상)` 로 켜고 끄기, "관전 중: 이름", PlayerState `Get Respawn Time Remaining` 카운트다운
- [ ] 점수판 (Tab) — GameState `Get Sorted Players`, `On Scoreboard Changed`. 봇은 PlayerState `Is Bot` 으로 표시
- [ ] 매치 결과 화면 — GameState `On Match Phase Changed` 가 Ended 일 때 `Get Winners`. 10초 뒤 자동으로 새 매치
- [ ] (나중) 세션 메뉴 — Session Subsystem 의 On Host/Find/Join/Leave Complete, `Get Last Search Results`

---

## 11. 테스트 순서

### 11-1. 혼자 PIE (손맛)
- [ ] 발밑 사격 → 로켓 점프
- [ ] 발밑 사격 직후 점프 → 더 높이 뜸 (폭발 점프 유예)
- [ ] 벽 너머의 봇은 폭발에 안 밀림
- [ ] 리스폰 직후 무적: 남의 탄이 몸을 통과, 내가 쏘면 무적 해제
- [ ] 넉백 직후 키를 눌러도 넉백이 상쇄되지 않음 (넉백 후 조작력 감소)
- [ ] 매치 종료 시 모두 그 자리에 멈춤 → 10초 뒤 새 매치 (확인하려면 `BP_PungGameMode` 의 Match Duration 을 잠깐 30 으로)

### 11-2. PIE 2인 (네트워크)
- [ ] Play 옵션: Number of Players = 2, Net Mode = Play As Listen Server
- [ ] 클라이언트 창에서 돌면서 쏴도 조준한 방향대로 터지는지
- [ ] 쏜 사람 화면에서 탄이 바로 나가는지, 상대 화면에도 보이는지
- [ ] 밀려날 때 끊김 (필요하면 `pung.Knockback.ClientApply 0/1` 비교)
- [ ] 발밑 사격 직후 점프할 때 위치가 튀는지 (튀면 Claude 에게: 폭발 점프 유예 네트워크 보정 필요)

### 11-3. Standalone + Steam (두 PC, 서로 다른 스팀 계정)
- [ ] 두 PC 모두 Steam 실행 후 `Run-Standalone.bat`
- [ ] 오른쪽 아래 Steam 오버레이 알림 (Shift+Tab) 확인
- [ ] PC1: `PungHost` → `PungSession` 에서 호스트=1 등록=1
- [ ] PC2: `PungFind` → `PungJoin 0`
- [ ] 같이 플레이, 킬/점수, 매치 종료 후 재시작 때 PC2 가 따라오는지 (끊기면 Claude 에게: 심리스 트래블 필요)
- [ ] `PungInvite` 로 친구 초대 → 수락 시 참가
- [ ] PC1 `PungLeave` → PC2 가 끊기고 기본 맵으로 돌아오는지
- [ ] (선택) 한 PC 에서 `Run-Standalone.bat nosteam` 두 개로 로컬 접속 테스트

---

## 12. 마무리

- [ ] 에디터에서 만든 에셋 (BP, DA, IA, BB, BT, 맵) 커밋. `.uasset` / `.umap` 은 Git LFS 로 올라간다
- [ ] 이 브랜치를 `main` 에 합치는 PR 만들기
- [ ] 테스트하며 바꾼 수치가 있으면 GDD §10 갱신 (또는 Claude 에게 알려 갱신)
- [ ] GDD §11 M1 체크리스트 갱신

---

## Claude 에게 알려줄 것 (확인이 필요한 부분)

- [ ] 빌드 결과 (특히 아이템, UI 데이터 코드는 처음 빌드)
- [ ] 아이템 효과 드롭다운에 한글 이름이 깨지는지
- [ ] 관전 카메라 전환이 매끄러운지
- [ ] 봇이 조준할 때 고개가 위아래로 안 움직이는지 (엔진 특성. 판정에는 영향 없음)
- [ ] 폭발 점프 유예 후 위치 튐 여부
- [ ] 매치 재시작 때 클라이언트가 따라오는지
- [ ] 테스트하며 느낀 손맛 (넉백 세기, 반경, 공중 제어 등) → 튜닝
