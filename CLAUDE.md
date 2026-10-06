# Pung

UE 5.8 (C++ + Blueprint) 1인칭 멀티플레이어 링아웃 슈터. 기획과 결정 사항은 `Docs/GDD.md` 가 기준이다.

## 언어
- 커밋 메시지, 코드 주석, 로그는 **한국어**로 쓴다.
- `Config/*.ini` 는 ASCII 만 쓴다 (한글 주석 금지).

## GDD
- `Docs/GDD.md` 는 살아있는 문서다. 기능이나 규칙을 바꾸면 같은 커밋에서 GDD 도 갱신한다.
  - 해당 섹션 본문, §10 튜닝 파라미터(새 수치), §10.5 코드 구조(새 클래스), §12 결정 기록(한 줄)
- 웹 원작(PUNG!) 에서는 로직과 설계 의도만 가져오고 수치는 옮기지 않는다 (§13).

## 코드 구조
- `Source/Pung/Public|Private/<영역>/` (Character, Weapon, Game, Player, Online)
- 캐릭터와 게임 로직은 C++ 중심, BP 는 에셋 지정용 얇은 껍데기.
- 넉백, 킬 판정, 점수는 서버 권한. 리슨 서버이므로 서버에서 값을 바꿀 때 `OnRep_` 을 직접 호출해 호스트 화면도 갱신한다.
- 무기 성능 수치는 `UPungAirGunData` 에만 둔다. 외형(스킨) 데이터는 별도 에셋으로 분리 (GDD §3.5).
- 튜닝 값은 `EditAnywhere`/`EditDefaultsOnly` 로 노출하고 단위 메타(`Units=`)를 붙인다.
- 디버그용 콘솔 변수는 `pung.` 접두사, 콘솔 명령은 `APungPlayerController` 의 `Pung*` Exec 함수.

## 작업 환경
- 콘텐츠(`.uasset`, `.umap`) 는 Git LFS. 클라우드 세션에서는 포인터만 보이므로 BP/맵 내용은 확인할 수 없다.
- 클라우드 세션에서는 빌드할 수 없다. 사용자가 로컬에서 `Build-Editor.bat` 으로 빌드하고, Steam 세션 테스트는 `Run-Standalone.bat` 으로 한다 (PIE 에서는 Steam 이 동작하지 않음). 확인이 필요한 엔진 API 는 작업 보고에 적어둔다.
- UI 는 UserWidget(UMG) 기반으로 사용자가 직접 만든다. AHUD 는 쓰지 않는다. C++ 쪽은 델리게이트와 BlueprintPure 게터로 데이터를 노출한다.
