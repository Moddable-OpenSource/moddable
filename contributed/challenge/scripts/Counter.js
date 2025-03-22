function* counter() {
  for (let i = 0; true; i++) {
    const guessedCode = i.toString(36).toUpperCase().padStart(10, "0");
    guess(guessedCode);
    yield;
  }
}

