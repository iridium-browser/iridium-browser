// A normalizing helper function to try and compare extracted marks in a way
// that's robust-ish to reformatting/prettifying of the HTML input files.
export const marksArrayToString = (marks) => {
  // Extract text content with normalized whitespace
  const text =
      marks.map((node) => node.textContent.replace(/[\t\n\r ]+/g, ' '));
  // Join, re-normalize (because block elements can add extra whitespace if
  // HTML contains newlines), and trim.
  return text.join('').replace(/[\t\n\r ]+/g, ' ').trim();
};
