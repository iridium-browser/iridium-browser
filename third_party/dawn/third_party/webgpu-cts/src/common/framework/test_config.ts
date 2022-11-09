export type TestConfig = {
  maxSubcasesInFlight: number;
  testHeartbeatCallback: () => void;
  noRaceWithRejectOnTimeout: boolean;
};

export const globalTestConfig: TestConfig = {
  maxSubcasesInFlight: 500,
  testHeartbeatCallback: () => {},
  noRaceWithRejectOnTimeout: false,
};
