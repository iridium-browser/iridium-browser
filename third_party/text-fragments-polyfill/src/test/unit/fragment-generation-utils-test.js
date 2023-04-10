import * as generationUtils from '../../src/fragment-generation-utils.js';
import * as fragmentUtils from '../../src/text-fragment-utils.js';

/**
 * Tests should call this method immediately if they rely on Intl.Segmenter
 * support. This will cause them to be skipped on platforms that don't have
 * the needed functionality.
 */
function requireIntlSegmenterSupport() {
  if (!Intl.Segmenter) {
    pending('This configuration does not yet support Intl.Segmenter.');
    return;
  }
};

/**
 * Helper function to determine if tests are running for debugging
 * @return {Boolean} - true if the --debug flag was passed to karma start
 */
function isDebug() {
  return __karma__.config.debug;
};

describe('FragmentGenerationUtils', function() {
  beforeEach(function() {
    generationUtils.setTimeout(isDebug() ? null : 500);
    generationUtils.forTesting.recordStartTime(Date.now());
  });

  it('can generate a fragment for an exact match', function() {
    document.body.innerHTML = __html__['basic-test.html'];
    const range = document.createRange();
    // firstChild of body is a <p>; firstChild of <p> is a text node.
    range.selectNodeContents(document.body.firstChild.firstChild);

    const selection = window.getSelection();
    selection.removeAllRanges();
    selection.addRange(range);

    const result = generationUtils.generateFragment(selection);
    expect(result.status)
        .toEqual(generationUtils.GenerateFragmentStatus.SUCCESS);
    expect(result.fragment.textStart).not.toBeUndefined();
    expect(result.fragment.textStart)
        .toEqual('this is a trivial test of the marking logic.');
    expect(result.fragment.textEnd).toBeUndefined();
    expect(result.fragment.prefix).toBeUndefined();
    expect(result.fragment.suffix).toBeUndefined();
  });

  it('can generate a fragment for a match across block boundaries', function() {
    document.body.innerHTML = __html__['marks-test.html'];
    const range = document.createRange();

    range.setStart(document.getElementById('c'), 0);
    range.setEnd(document.getElementById('f'), 1);

    expect(fragmentUtils.forTesting.normalizeString(range.toString()))
        .toEqual('elaborate fancy div with lots of different stuff');

    const selection = window.getSelection();
    selection.removeAllRanges();
    selection.addRange(range);

    let result = generationUtils.generateFragment(selection);
    expect(result.status)
        .toEqual(generationUtils.GenerateFragmentStatus.SUCCESS);
    expect(result.fragment.textStart).toEqual('elaborate');
    expect(result.fragment.textEnd).toEqual('of different stuff');
    expect(result.fragment.prefix).toBeUndefined();
    expect(result.fragment.suffix).toBeUndefined();

    range.selectNodeContents(document.getElementById('a'));

    expect(fragmentUtils.forTesting.normalizeString(range.toString().trim()))
        .toEqual(
            'this is a really elaborate fancy div with lots of different stuff in it.');

    selection.removeAllRanges();
    selection.addRange(range);
    result = generationUtils.generateFragment(selection);
    expect(result.status)
        .toEqual(generationUtils.GenerateFragmentStatus.SUCCESS);
    expect(result.fragment.textStart).toEqual('This is');
    expect(result.fragment.textEnd).toEqual('stuff\n  in it.');
    expect(result.fragment.prefix).toBeUndefined();
    expect(result.fragment.suffix).toBeUndefined();
  });

  it('can generate a fragment for a really long range in a text node.',
     function() {
       document.body.innerHTML = __html__['very-long-text.html'];
       const range = document.createRange();
       range.selectNodeContents(document.getElementById('root'));

       const selection = window.getSelection();
       selection.removeAllRanges();
       selection.addRange(range);

       const result = generationUtils.generateFragment(selection);
       expect(result.fragment.textStart).toEqual('words words words');
       expect(result.fragment.textEnd).toEqual('words words words');
     });

  it('respects br tags when generating fragments', function() {
    document.body.innerHTML = __html__['br-tags.html'];

    const target = document.createRange();
    const selection = window.getSelection();
    // Choose start/end points to include a couple inline <br>s
    const startnode = document.body.firstChild.childNodes[6];
    const endnode = document.body.firstChild.childNodes[7];
    target.setStart(startnode, 0);
    target.setEnd(endnode, 1);
    selection.removeAllRanges();
    selection.addRange(target);

    const result = generationUtils.generateFragment(selection);
    expect(result.status)
        .toEqual(generationUtils.GenerateFragmentStatus.SUCCESS);
    expect(result.fragment.textStart).toEqual('elle a brise');
  });

  it('handles a prefix without a suffix', function() {
    document.body.innerHTML = __html__['prefix-suffix-at-edges.html'];

    const target = document.createRange();
    const selection = window.getSelection();
    const startnode = document.body.firstChild.firstChild;
    target.setStart(startnode, 21);
    target.setEnd(startnode, 27);
    selection.removeAllRanges();
    selection.addRange(target);

    const result = generationUtils.generateFragment(selection);
    expect(result.status)
        .toEqual(generationUtils.GenerateFragmentStatus.SUCCESS);
    expect(result.fragment.textStart).toEqual('target');
    expect(result.fragment.prefix).toEqual('prefix');
    expect(result.fragment.suffix).toBeUndefined();
  });

  it('handles a suffix without a prefix', function() {
    document.body.innerHTML = __html__['prefix-suffix-at-edges.html'];

    const target = document.createRange();
    const selection = window.getSelection();
    const startnode = document.body.firstChild.firstChild;
    target.setStart(startnode, 0);
    target.setEnd(startnode, 6);
    selection.removeAllRanges();
    selection.addRange(target);

    const result = generationUtils.generateFragment(selection);
    expect(result.status)
        .toEqual(generationUtils.GenerateFragmentStatus.SUCCESS);
    expect(result.fragment.textStart).toEqual('target');
    expect(result.fragment.suffix).toEqual('suffix');
    expect(result.fragment.prefix).toBeUndefined();
  });

  it('can detect if a range contains a block boundary', function() {
    document.body.innerHTML = __html__['marks-test.html'];
    const range = document.createRange();
    const root = document.getElementById('a');

    // Starts/ends inside text nodes that are children of the same block element
    // and have block elements in between them.
    range.setStart(root.firstChild, 3);
    range.setEnd(root.lastChild, 5);
    expect(generationUtils.forTesting.containsBlockBoundary(range))
        .toEqual(true);

    // Starts/ends inside a single text node.
    range.setStart(root.firstChild, 3);
    range.setEnd(root.firstChild, 7);
    expect(generationUtils.forTesting.containsBlockBoundary(range))
        .toEqual(false);

    // Contains other nodes, but none of them are block nodes.
    range.setStart(root.childNodes[4], 3);  // "div with"
    range.setEnd(root.lastChild, 5);
    expect(generationUtils.forTesting.containsBlockBoundary(range))
        .toEqual(false);

    // Detects boundaries that are only the start of a block node.
    range.setStart(root.firstChild, 3);
    range.setEnd(document.getElementById('b').firstChild, 5);  // "a really"
    expect(generationUtils.forTesting.containsBlockBoundary(range))
        .toEqual(true);

    // Detects boundaries that are only the end of a block node.
    range.setStart(document.getElementById('e').firstChild, 1);  // "fancy"
    range.setEnd(root.childNodes[4], 7);                         // "div with"
    expect(generationUtils.forTesting.containsBlockBoundary(range))
        .toEqual(true);
  });

  it('can find a word start inside a text node', function() {
    document.body.innerHTML = __html__['word-bounds-test.html'];
    const elt = document.getElementById('block');

    // elt is an HTML element, not a text node, so we should find -1
    let result = generationUtils.forTesting.findWordStartBoundInTextNode(elt);
    expect(result).toEqual(-1);

    const node = elt.firstChild;
    // With no second arg, we find the first from the end
    result = generationUtils.forTesting.findWordStartBoundInTextNode(node);
    expect(result).toEqual(7);  // Between " " and "b"

    // Second arg in the middle of a word
    result = generationUtils.forTesting.findWordStartBoundInTextNode(node, 10);
    expect(result).toEqual(7);  // Between " " and "b"

    // Second arg immediately *before* a space should give the same output
    result = generationUtils.forTesting.findWordStartBoundInTextNode(node, 6);
    expect(result).toEqual(6)

    // No more spaces to the left of second arg, -1
    result = generationUtils.forTesting.findWordStartBoundInTextNode(node, 3);
    expect(result).toEqual(-1);
  });

  it('can find a word end inside a text node', function() {
    document.body.innerHTML = __html__['word-bounds-test.html'];
    const elt = document.getElementById('block');

    // elt is an HTML element, not a text node, so we should find -1
    let result = generationUtils.forTesting.findWordEndBoundInTextNode(elt);
    expect(result).toEqual(-1);

    const node = elt.firstChild;
    // With no second arg, we find the first
    result = generationUtils.forTesting.findWordEndBoundInTextNode(node);
    expect(result).toEqual(6);  // Between "e" and " "

    // Second arg in the middle of a word
    result = generationUtils.forTesting.findWordEndBoundInTextNode(node, 2);
    expect(result).toEqual(6);  // Between "e" and " "

    // Second arg immediately *after* a space should give the same output
    result = generationUtils.forTesting.findWordEndBoundInTextNode(node, 7);
    expect(result).toEqual(7)

    // No more spaces to the right of second arg, -1
    result = generationUtils.forTesting.findWordEndBoundInTextNode(node, 10);
    expect(result).toEqual(-1);
  });

  it('can expand a range start to a word bound within a node', function() {
    document.body.innerHTML = __html__['word-bounds-test.html'];
    const range = document.createRange();
    const textNodeInBlock = document.getElementById('block').firstChild;

    range.setStart(textNodeInBlock, 10);
    range.setEnd(textNodeInBlock, 12);
    expect(range.toString()).toEqual('ck');

    generationUtils.forTesting.expandRangeStartToWordBound(range);
    expect(range.toString()).toEqual('block');

    // Repeat the process and expect it to be a no-op.
    generationUtils.forTesting.expandRangeStartToWordBound(range);
    expect(range.toString()).toEqual('block');
  });

  it('can expand a range start to a word bound across nodes in Japanese',
     function() {
       requireIntlSegmenterSupport();

       document.body.innerHTML = __html__['jp-word-boundary.html'];
       const range = document.createRange();
       // The last text node contains a partial word and needs to include a char
       // from the preceding <i> tag to make a full word.
       const wordSuffix = document.getElementById('root').lastChild;
       range.selectNodeContents(wordSuffix);
       expect(range.toString()).toEqual('リーズ');

       generationUtils.forTesting.expandRangeStartToWordBound(range);
       expect(range.toString()).toEqual('シリーズ');

       // This node contains the end of one word and the start of another, but
       // in context is not a full word itself.
       const innerText = document.getElementById('i').firstChild;
       range.selectNodeContents(innerText);
       expect(range.toString()).toEqual('ックシ');

       generationUtils.forTesting.expandRangeStartToWordBound(range);
       expect(range.toString()).toEqual('ソニックシ');
     });

  it('can expand a range end to a word bound within a node', function() {
    document.body.innerHTML = __html__['word-bounds-test.html'];
    const range = document.createRange();
    const textNodeInBlock = document.getElementById('block').firstChild;

    range.setStart(textNodeInBlock, 0);
    range.setEnd(textNodeInBlock, 3);
    expect(range.toString()).toEqual('Ins');

    generationUtils.forTesting.expandRangeEndToWordBound(range);
    expect(range.toString()).toEqual('Inside');

    // Repeat the process and expect it to be a no-op.
    generationUtils.forTesting.expandRangeEndToWordBound(range);
    expect(range.toString()).toEqual('Inside');
  });

  it('can expand a range start to an inner block boundary', function() {
    document.body.innerHTML = __html__['word-bounds-test.html'];
    const range = document.createRange();
    const textNodeInBlock = document.getElementById('block').firstChild;

    range.setStart(textNodeInBlock, 3);
    range.setEnd(textNodeInBlock, 12);
    expect(range.toString()).toEqual('ide block');

    generationUtils.forTesting.expandRangeStartToWordBound(range);
    expect(range.toString()).toEqual('Inside block');

    expect(generationUtils.forTesting.containsBlockBoundary(range))
        .toEqual(false);
  });

  it('can expand a range end to an inner block boundary', function() {
    document.body.innerHTML = __html__['word-bounds-test.html'];
    const range = document.createRange();
    const textNodeInBlock = document.getElementById('block').firstChild;

    range.setStart(textNodeInBlock, 0);
    range.setEnd(textNodeInBlock, 10);
    expect(range.toString()).toEqual('Inside blo');

    generationUtils.forTesting.expandRangeEndToWordBound(range);
    expect(range.toString()).toEqual('Inside block');

    expect(generationUtils.forTesting.containsBlockBoundary(range))
        .toEqual(false);
  });

  it('can expand a range end across inline elements', function() {
    document.body.innerHTML = __html__['word-bounds-test.html'];
    const range = document.createRange();
    const inlineTextNode = document.getElementById('inline').firstChild;
    // Get the text node between the <p> and <i> nodes:
    const middleTextNode = document.getElementById('root').childNodes[2];

    range.setStart(middleTextNode, 3);
    range.setEnd(inlineTextNode, 2);
    expect(range.toString()).toEqual('Inli');

    generationUtils.forTesting.expandRangeEndToWordBound(range);
    expect(range.toString()).toEqual('Inline');

    range.setStart(middleTextNode, 3);
    range.setEnd(middleTextNode, 5);
    expect(range.toString()).toEqual('In');

    generationUtils.forTesting.expandRangeEndToWordBound(range);
    expect(range.toString()).toEqual('Inline');
  });

  it('can find the search space for range-based fragments', function() {
    document.body.innerHTML = __html__['marks-test.html'];
    const range = document.createRange();

    // Simplest case: a whole element with a bunch of block boundaries inside.
    range.selectNodeContents(document.getElementById('a'));
    expect(fragmentUtils.forTesting.normalizeString(
               generationUtils.forTesting.getSearchSpaceForStart(range)))
        .toEqual('this is');
    expect(fragmentUtils.forTesting.normalizeString(
               generationUtils.forTesting.getSearchSpaceForEnd(range)))
        .toEqual('div with lots of different stuff in it.');

    // Starts and ends inside a text node. No block boundaries, so we should get
    // an undefined return.
    range.selectNodeContents(document.getElementById('e').firstChild);
    expect(generationUtils.forTesting.getSearchSpaceForStart(range))
        .toBeUndefined();
    expect(generationUtils.forTesting.getSearchSpaceForEnd(range))
        .toBeUndefined();


    // Starts inside one block, ends outside that block
    range.selectNodeContents(document.getElementById('a'));
    range.setStart(document.getElementById('c').firstChild, 0);
    expect(fragmentUtils.forTesting.normalizeString(
               generationUtils.forTesting.getSearchSpaceForStart(range)))
        .toEqual('elaborate');
    expect(fragmentUtils.forTesting.normalizeString(
               generationUtils.forTesting.getSearchSpaceForEnd(range)))
        .toEqual('div with lots of different stuff in it.');

    // Ends inside one block, started outside that block
    range.selectNodeContents(document.getElementById('a'));
    range.setEnd(document.getElementById('b').lastChild, 3);
    expect(fragmentUtils.forTesting.normalizeString(
               generationUtils.forTesting.getSearchSpaceForStart(range)))
        .toEqual('this is');
    expect(fragmentUtils.forTesting.normalizeString(
               generationUtils.forTesting.getSearchSpaceForEnd(range)))
        .toEqual('a really elaborate');

    // Starts and ends in different, non-overlapping divs
    range.setStart(document.getElementById('c').firstChild, 0);
    range.setEnd(document.getElementById('e').firstChild, 5);
    expect(fragmentUtils.forTesting.normalizeString(
               generationUtils.forTesting.getSearchSpaceForStart(range)))
        .toEqual('elaborate');
    expect(fragmentUtils.forTesting.normalizeString(
               generationUtils.forTesting.getSearchSpaceForEnd(range)))
        .toEqual('fancy');

    // Boundaries that aren't text nodes
    range.setStart(document.getElementById('a'), 1);
    range.setEnd(document.getElementById('a'), 6);
    expect(fragmentUtils.forTesting.normalizeString(
               generationUtils.forTesting.getSearchSpaceForStart(range)))
        .toEqual('a really elaborate');
    expect(fragmentUtils.forTesting.normalizeString(
               generationUtils.forTesting.getSearchSpaceForEnd(range)))
        .toEqual('div with lots of different stuff');

    // End boundary is a series of non-word characters at the end of a block
    document.body.innerHTML = __html__['non-word-boundaries.html'];
    range.setStart(document.getElementById('em').firstChild, 4);
    range.setEnd(document.body, 3);
    expect(generationUtils.forTesting.getSearchSpaceForStart(range))
        .toEqual('...');
  });

  it('excludes invisible text nodes when getting search space', function() {
    document.body.innerHTML = __html__['invisible-text-node.html'];

    const range = document.createRange();
    range.setStartBefore(document.getElementById('prefix'));
    range.setEndAfter(document.getElementById('prefix'));

    const prefixSearchSpace =
        generationUtils.forTesting.getSearchSpaceForEnd(range);
    expect(prefixSearchSpace).toEqual('visible1');

    range.setStartBefore(document.getElementById('suffix'));
    range.setEndAfter(document.getElementById('suffix'));

    const suffixSearchSpace =
        generationUtils.forTesting.getSearchSpaceForEnd(range);
    expect(suffixSearchSpace).toEqual('visible2');
  });

  it('can generate progressively larger fragments across blocks', function() {
    document.body.innerHTML = __html__['range-fragment-test.html'];
    const range = document.createRange();
    range.selectNodeContents(document.getElementById('root'));

    const startSpace = generationUtils.forTesting.getSearchSpaceForStart(range);
    const endSpace = generationUtils.forTesting.getSearchSpaceForEnd(range);

    const factory =
        new generationUtils.forTesting.FragmentFactory()
            .setStartAndEndSearchSpace(startSpace, endSpace)
            .useSegmenter(fragmentUtils.forTesting.makeNewSegmenter());

    expect(factory.embiggen()).toEqual(true);
    expect(startSpace.substring(0, factory.startOffset))
        .toEqual('repeat repeat repeat');
    expect(endSpace.substring(factory.endOffset))
        .toEqual('repeat repeat repeat');

    expect(factory.tryToMakeUniqueFragment()).toBeUndefined();

    expect(factory.embiggen()).toEqual(true);
    expect(startSpace.substring(0, factory.startOffset))
        .toEqual('repeat repeat repeat unique');
    expect(endSpace.substring(factory.endOffset))
        .toEqual('unique repeat repeat repeat');

    const fragment = factory.tryToMakeUniqueFragment();
    expect(fragment).not.toBeUndefined();
    expect(fragment.textStart).toEqual('repeat repeat repeat unique');
    expect(fragment.textEnd).toEqual('unique repeat repeat repeat');

    expect(factory.embiggen()).toEqual(true);
    expect(startSpace.substring(0, factory.startOffset))
        .toEqual('repeat repeat repeat unique repeat');
    expect(endSpace.substring(factory.endOffset))
        .toEqual('repeat unique repeat repeat repeat');

    expect(factory.embiggen()).toEqual(true);
    expect(factory.embiggen()).toEqual(true);

    expect(factory.embiggen()).toEqual(false);
  });


  it('can generate progressively larger fragments across blocks in Japanese',
     function() {
       requireIntlSegmenterSupport();

       document.body.innerHTML = __html__['range-fragment-jp.html'];
       const range = document.createRange();
       range.selectNodeContents(document.getElementById('root'));

       const startSpace =
           generationUtils.forTesting.getSearchSpaceForStart(range);
       const endSpace = generationUtils.forTesting.getSearchSpaceForEnd(range);

       const factory =
           new generationUtils.forTesting.FragmentFactory()
               .setStartAndEndSearchSpace(startSpace, endSpace)
               .useSegmenter(fragmentUtils.forTesting.makeNewSegmenter());

       expect(factory.embiggen()).toEqual(true);
       expect(startSpace.substring(0, factory.startOffset))
           .toEqual('いただきますいただきますいただきます');
       expect(endSpace.substring(factory.endOffset))
           .toEqual('いただきますいただきますいただきます');

       expect(factory.tryToMakeUniqueFragment()).toBeUndefined();

       expect(factory.embiggen()).toEqual(true);
       expect(startSpace.substring(0, factory.startOffset))
           .toEqual('いただきますいただきますいただきますご馳走様');
       expect(endSpace.substring(factory.endOffset))
           .toEqual('ご馳走様いただきますいただきますいただきます');

       const fragment = factory.tryToMakeUniqueFragment();
       expect(fragment).not.toBeUndefined();
       expect(fragment.textStart)
           .toEqual('いただきますいただきますいただきますご馳走様');
       expect(fragment.textEnd)
           .toEqual('ご馳走様いただきますいただきますいただきます');

       expect(factory.embiggen()).toEqual(true);
       expect(startSpace.substring(0, factory.startOffset))
           .toEqual('いただきますいただきますいただきますご馳走様いただきます');
       expect(endSpace.substring(factory.endOffset))
           .toEqual('いただきますご馳走様いただきますいただきますいただきます');

       expect(factory.embiggen()).toEqual(true);
       expect(factory.embiggen()).toEqual(true);

       expect(factory.embiggen()).toEqual(false);
     });

  it('can generate progressively larger fragments within a block', function() {
    const sharedSpace =
        'text1 text2  text3 text4, text5 text6 text7 text8 text9 text10';

    const factory =
        new generationUtils.forTesting.FragmentFactory()
            .setSharedSearchSpace(sharedSpace)
            .useSegmenter(fragmentUtils.forTesting.makeNewSegmenter());

    expect(factory.embiggen()).toEqual(true);
    expect(sharedSpace.substring(0, factory.startOffset))
        .toEqual('text1 text2  text3');
    expect(sharedSpace.substring(factory.endOffset))
        .toEqual('text8 text9 text10');

    expect(factory.embiggen()).toEqual(true);
    expect(sharedSpace.substring(0, factory.startOffset))
        .toEqual('text1 text2  text3 text4');
    expect(sharedSpace.substring(factory.endOffset))
        .toEqual('text7 text8 text9 text10');

    expect(factory.embiggen()).toEqual(true);
    expect(sharedSpace.substring(0, factory.startOffset))
        .toEqual('text1 text2  text3 text4, text5');
    expect(sharedSpace.substring(factory.endOffset))
        .toEqual('text6 text7 text8 text9 text10');

    expect(factory.embiggen()).toEqual(true);
    expect(sharedSpace.substring(0, factory.startOffset))
        .toEqual('text1 text2  text3 text4, text5 ');
    expect(sharedSpace.substring(factory.endOffset))
        .toEqual('text6 text7 text8 text9 text10');

    expect(factory.embiggen()).toEqual(false);
  });

  it('can generate progressively larger fragments within a block in Japanese',
     function() {
       requireIntlSegmenterSupport();

       const sharedSpace = 'メガドライブゲームギアソニックドリームキャスト';

       const factory =
           new generationUtils.forTesting.FragmentFactory()
               .setSharedSearchSpace(sharedSpace)
               .useSegmenter(fragmentUtils.forTesting.makeNewSegmenter());

       expect(factory.embiggen()).toEqual(true);
       expect(sharedSpace.substring(0, factory.startOffset))
           .toEqual('メガドライブゲーム');
       expect(sharedSpace.substring(factory.endOffset))
           .toEqual('ソニックドリームキャスト');

       expect(factory.embiggen()).toEqual(true);
       expect(sharedSpace.substring(0, factory.startOffset))
           .toEqual('メガドライブゲームギア');
       expect(sharedSpace.substring(factory.endOffset))
           .toEqual('ソニックドリームキャスト');

       expect(factory.embiggen()).toEqual(false);
     });

  it('can add context to a single-block range match', function() {
    const sharedSpace = 'text1 text2 text3 text4 text5 text6 text7';
    const prefixSpace = 'prefix3 prefix2 prefix1';
    const suffixSpace = 'suffix1 suffix2 suffix3';

    const factory =
        new generationUtils.forTesting.FragmentFactory()
            .setSharedSearchSpace(sharedSpace)
            .setPrefixAndSuffixSearchSpace(prefixSpace, suffixSpace)
            .useSegmenter(fragmentUtils.forTesting.makeNewSegmenter());

    expect(factory.embiggen()).toEqual(true);
    expect(sharedSpace.substring(0, factory.startOffset))
        .toEqual('text1 text2 text3');
    expect(sharedSpace.substring(factory.endOffset))
        .toEqual('text5 text6 text7');

    expect(factory.embiggen()).toEqual(true);
    expect(sharedSpace.substring(0, factory.startOffset))
        .toEqual('text1 text2 text3 text4');
    expect(sharedSpace.substring(factory.endOffset))
        .toEqual(' text5 text6 text7');
    expect(prefixSpace.substring(factory.prefixOffset))
        .toEqual('prefix3 prefix2 prefix1');
    expect(suffixSpace.substring(0, factory.suffixOffset))
        .toEqual('suffix1 suffix2 suffix3');

    expect(factory.embiggen()).toEqual(false);
  });

  it('can add context to an exact text match', function() {
    const exactText = 'text1 text2 text3 text4 text5 text6 text7';
    const prefixSpace = 'prefix4 prefix3 prefix2 prefix1';
    const suffixSpace = 'suffix1 suffix2 suffix3 suffix4';

    const factory =
        new generationUtils.forTesting.FragmentFactory()
            .setExactTextMatch(exactText)
            .setPrefixAndSuffixSearchSpace(prefixSpace, suffixSpace)
            .useSegmenter(fragmentUtils.forTesting.makeNewSegmenter());

    expect(factory.embiggen()).toEqual(true);
    expect(factory.exactTextMatch).toEqual(exactText);
    expect(prefixSpace.substring(factory.prefixOffset))
        .toEqual('prefix3 prefix2 prefix1');
    expect(suffixSpace.substring(0, factory.suffixOffset))
        .toEqual('suffix1 suffix2 suffix3');

    expect(factory.embiggen()).toEqual(true);
    expect(factory.exactTextMatch).toEqual(exactText);
    expect(prefixSpace.substring(factory.prefixOffset))
        .toEqual('prefix4 prefix3 prefix2 prefix1');
    expect(suffixSpace.substring(0, factory.suffixOffset))
        .toEqual('suffix1 suffix2 suffix3 suffix4');

    expect(factory.embiggen()).toEqual(false);
  });

  it('can add context to an exact text match in Japanese', function() {
    requireIntlSegmenterSupport();

    const exactText = 'ソニック';
    const prefixSpace = 'メガドライブゲームギア';
    const suffixSpace = 'ドリームキャスト';

    const factory =
        new generationUtils.forTesting.FragmentFactory()
            .setExactTextMatch(exactText)
            .setPrefixAndSuffixSearchSpace(prefixSpace, suffixSpace)
            .useSegmenter(fragmentUtils.forTesting.makeNewSegmenter());

    expect(factory.embiggen()).toEqual(true);
    expect(factory.exactTextMatch).toEqual(exactText);
    expect(prefixSpace.substring(factory.prefixOffset))
        .toEqual('ドライブゲームギア');
    expect(suffixSpace.substring(0, factory.suffixOffset))
        .toEqual('ドリームキャスト');

    expect(factory.embiggen()).toEqual(true);
    expect(factory.exactTextMatch).toEqual(exactText);
    expect(prefixSpace.substring(factory.prefixOffset))
        .toEqual('メガドライブゲームギア');
    expect(suffixSpace.substring(0, factory.suffixOffset))
        .toEqual('ドリームキャスト');

    expect(factory.embiggen()).toEqual(false);
  });

  it('can generate prefixes/suffixes to distinguish short matches', function() {
    // This is the most common case for prefix/suffix matches: a user selects a
    // word or a small portion of a word.
    document.body.innerHTML = __html__['ambiguous-match.html'];

    const target = document.createRange();
    const selection = window.getSelection();

    target.selectNodeContents(document.getElementById('target1'));
    selection.removeAllRanges();
    selection.addRange(target);

    let result = generationUtils.generateFragment(selection);
    expect(result.fragment.textStart).toEqual('target');
    expect(result.fragment.textEnd).toBeUndefined();
    expect(result.fragment.prefix).toEqual('prefix1');
    expect(result.fragment.suffix).toEqual('suffix1 prefix2\n  target');

    target.selectNodeContents(document.getElementById('target3'));
    selection.removeAllRanges();
    selection.addRange(target);

    result = generationUtils.generateFragment(selection);
    expect(result.fragment.textStart).toEqual('target');
    expect(result.fragment.textEnd).toBeUndefined();
    expect(result.fragment.prefix).toEqual('target suffix2 prefix1');
    expect(result.fragment.suffix).toEqual('suffix2\n  prefix2 target');
  });

  it('can generate prefixes/suffixes to distinguish long matches', function() {
    // A passage which appears multiple times on a page.
    document.body.innerHTML = __html__['long-ambiguous-match.html'];

    const target = document.createRange();
    const selection = window.getSelection();

    const node = document.getElementById('target').firstChild;
    target.setStart(node, 5);
    target.setEnd(node, node.textContent.length - 8);
    selection.removeAllRanges();
    selection.addRange(target);

    const result = generationUtils.generateFragment(selection);
    expect(result.fragment.textStart).not.toBeFalsy();
    expect(result.fragment.textEnd).not.toBeFalsy();
    expect(fragmentUtils.forTesting.normalizeString(result.fragment.prefix))
        .toEqual('prefix. lorem ipsum dolor');
    expect(fragmentUtils.forTesting.normalizeString(result.fragment.suffix))
        .toEqual('recteque qui ei. suffix');
  });

  it('can generate URLs spanning table elements', function() {
    document.body.innerHTML = __html__['table.html'];

    const target = document.createRange();
    const selection = window.getSelection();

    const nodeA = document.getElementById('a').firstChild;
    target.setStart(nodeA, 6);
    target.setEnd(nodeA, 11);
    selection.removeAllRanges();
    selection.addRange(target);

    let result = generationUtils.generateFragment(selection);
    expect(fragmentUtils.forTesting.normalizeString(result.fragment.prefix))
        .toEqual('first');
    expect(fragmentUtils.forTesting.normalizeString(result.fragment.textStart))
        .toEqual('named');

    const nodeB = document.getElementById('b').firstChild;
    target.setStart(nodeA, 0);
    target.setEnd(nodeB, nodeB.textContent.length);
    selection.removeAllRanges();
    selection.addRange(target);

    result = generationUtils.generateFragment(selection);
    expect(fragmentUtils.forTesting.normalizeString(result.fragment.textStart))
        .toEqual('first named entity');
    expect(fragmentUtils.forTesting.normalizeString(result.fragment.textEnd))
        .toEqual('june 3, 2014');
  });

  it('will halt generation after a certain time period', function() {
    if (isDebug()) {
      pending('Disabled for debugging');
    }

    document.body.innerHTML = __html__['basic-test.html'];
    const range = document.createRange();
    range.selectNodeContents(document.body.firstChild.firstChild);

    const selection = window.getSelection();
    selection.removeAllRanges();
    selection.addRange(range);

    expect(function() {
      generationUtils.forTesting.doGenerateFragment(
          selection, Date.now() - 1000);
    }).toThrowMatching(function(thrown) {
      return thrown.isTimeout
    });

    generationUtils.setTimeout(2000);
    expect(function() {
      generationUtils.forTesting.doGenerateFragment(
          selection, Date.now() - 1000);
    }).not.toThrowMatching(function(thrown) {
      return thrown.isTimeout
    });
  });


  it('will halt search space creation after a certain time period', function() {
    if (isDebug()) {
      pending('Disabled for debugging');
    }

    document.body.innerHTML = __html__['complicated-layout.html'];
    const range = document.createRange();
    range.selectNodeContents(document.getElementById('root'));

    generationUtils.forTesting.recordStartTime(Date.now() - 1000);
    expect(function() {
      generationUtils.forTesting.getSearchSpaceForStart(range)
    }).toThrowMatching(function(thrown) {
      return thrown.isTimeout
    });
  });

  it('identifies bad ranges as ineligible', async function() {
    document.body.innerHTML = __html__['range-eligibility.html'];
    const range = document.createRange();

    range.selectNodeContents(document.getElementById('regular-string'));
    expect(generationUtils.isValidRangeForFragmentGeneration(range)).toBeTrue();

    range.selectNodeContents(document.getElementById('inline-image'));
    expect(generationUtils.isValidRangeForFragmentGeneration(range)).toBeTrue();

    range.selectNodeContents(document.getElementById('punctuation-only'));
    expect(generationUtils.isValidRangeForFragmentGeneration(range))
        .toBeFalse();

    range.selectNodeContents(document.getElementById('inside-an-editable'));
    expect(generationUtils.isValidRangeForFragmentGeneration(range))
        .toBeFalse();

    range.selectNodeContents(document.getElementById('textarea'));
    expect(generationUtils.isValidRangeForFragmentGeneration(range))
        .toBeFalse();

    range.selectNodeContents(document.getElementById('input'));
    expect(generationUtils.isValidRangeForFragmentGeneration(range))
        .toBeFalse();

    const iframe = document.createElement('iframe');
    const setupIframe = new Promise((resolve, reject) => {
      iframe.srcdoc = '<p>test words</p>';
      iframe.onload = () => resolve();
      iframe.onerror = () => reject(new Error());
      document.body.appendChild(iframe);
    });
    await setupIframe;
    range.selectNodeContents(
        iframe.contentDocument.getElementsByTagName('p')[0]);
    expect(generationUtils.isValidRangeForFragmentGeneration(range))
        .toBeFalse();

    // Should work for a long (1000) string of punctuation with a regular
    // character at the end, but not a *really really* long (100k) one.
    const longString = document.createElement('p');
    let longStringContents = ' '.repeat(1000) + 'a';
    longString.textContent = longStringContents;
    range.selectNodeContents(longString);
    expect(generationUtils.isValidRangeForFragmentGeneration(range)).toBeTrue();
    longStringContents = ' '.repeat(100000) + 'a';
    longString.textContent = longStringContents;
    range.selectNodeContents(longString);
    expect(generationUtils.isValidRangeForFragmentGeneration(range))
        .toBeFalse();

    // Set up a really deep hierarchy and make sure we don't traverse the whole
    // thing
    let node = document.createElement('div');
    document.body.appendChild(node);
    for (let i = 0; i < 1000; i++) {
      const newNode = document.createElement('div');
      node.appendChild(newNode);
      node = newNode;
    }
    node.textContent = 'hello';
    range.selectNodeContents(node);
    expect(generationUtils.isValidRangeForFragmentGeneration(range))
        .toBeFalse();
  });

  it('can find text nodes in the same block as another node', function() {
    const extractor =
        (x) => {
          return x.textContent.trim();
        }

               document.body.innerHTML = __html__['text-node-extraction.html'];
    let node = document.getElementById('outer').childNodes[0];
    let result = generationUtils.forTesting.getTextNodesInSameBlock(node);
    expect(extractor(node)).toBe('0');
    expect(result.preNodes.map(extractor)).toEqual([]);
    expect(result.postNodes.map(extractor)).toEqual([]);

    node = document.getElementById('inner').childNodes[0];
    result = generationUtils.forTesting.getTextNodesInSameBlock(node);
    expect(extractor(node)).toBe('1');
    expect(result.preNodes.map(extractor)).toEqual([]);
    expect(result.postNodes.map(extractor)).toEqual(['2', '3', '4', '5']);

    node = document.getElementById('mid').childNodes[0];
    result = generationUtils.forTesting.getTextNodesInSameBlock(node);
    expect(extractor(node)).toBe('4');
    expect(result.preNodes.map(extractor)).toEqual(['1', '2', '3']);
    expect(result.postNodes.map(extractor)).toEqual(['5']);

    node = document.getElementById('outer').childNodes[2];
    result = generationUtils.forTesting.getTextNodesInSameBlock(node);
    expect(extractor(node)).toBe('8');
    expect(result.preNodes.map(extractor)).toEqual([]);
    expect(result.postNodes.map(extractor)).toEqual([]);
  });

  it('adds context on the first iteration for short range-based matches',
     function() {
       const startSpace = 'a .b  c d';
       const endSpace = 'w x \'y\' z';
       const prefixSpace = '4 3 2 1';
       const suffixSpace = '1 2 3 4';

       // The first iteration will add 7 characters each to the start and end,
       // and 14 is below the threshold (20), so context will be added on the
       // first iteration.

       const factory =
           new generationUtils.forTesting.FragmentFactory()
               .setStartAndEndSearchSpace(startSpace, endSpace)
               .setPrefixAndSuffixSearchSpace(prefixSpace, suffixSpace)
               .useSegmenter(fragmentUtils.forTesting.makeNewSegmenter());

       expect(factory.embiggen()).toEqual(true);
       expect(startSpace.substring(0, factory.startOffset)).toEqual('a .b  c');
       expect(endSpace.substring(factory.endOffset)).toEqual('x \'y\' z');
       expect(prefixSpace.substring(factory.prefixOffset)).toEqual('3 2 1');
       expect(suffixSpace.substring(0, factory.suffixOffset)).toEqual('1 2 3');

       expect(factory.embiggen()).toEqual(true);
       expect(startSpace.substring(0, factory.startOffset))
           .toEqual('a .b  c d');
       expect(endSpace.substring(factory.endOffset)).toEqual('w x \'y\' z');
       expect(prefixSpace.substring(factory.prefixOffset)).toEqual('4 3 2 1');
       expect(suffixSpace.substring(0, factory.suffixOffset))
           .toEqual('1 2 3 4');

       expect(factory.embiggen()).toEqual(false);
     });

  it('adds no context on the first iteration for long range-based matches',
     function() {
       const startSpace = 'internationalization .b  c d';
       const endSpace = 'w x \'y\' z';
       const prefixSpace = '4 3 2 1';
       const suffixSpace = '1 2 3 4';

       // "internationalization" is a 20-letter word, so any attempt at a
       // range-based match will easily exceed the 20-character threshold.
       // The first iteration should not include context.

       const factory =
           new generationUtils.forTesting.FragmentFactory()
               .setStartAndEndSearchSpace(startSpace, endSpace)
               .setPrefixAndSuffixSearchSpace(prefixSpace, suffixSpace)
               .useSegmenter(fragmentUtils.forTesting.makeNewSegmenter());

       expect(factory.embiggen()).toEqual(true);
       expect(startSpace.substring(0, factory.startOffset))
           .toEqual('internationalization .b  c');
       expect(endSpace.substring(factory.endOffset)).toEqual('x \'y\' z');
       expect(prefixSpace.substring(factory.prefixOffset)).toEqual('');
       expect(suffixSpace.substring(0, factory.suffixOffset)).toEqual('');

       expect(factory.embiggen()).toEqual(true);
       expect(startSpace.substring(0, factory.startOffset))
           .toEqual('internationalization .b  c d');
       expect(endSpace.substring(factory.endOffset)).toEqual('w x \'y\' z');
       expect(prefixSpace.substring(factory.prefixOffset)).toEqual('3 2 1');
       expect(suffixSpace.substring(0, factory.suffixOffset)).toEqual('1 2 3');

       expect(factory.embiggen()).toEqual(true);
       expect(startSpace.substring(0, factory.startOffset))
           .toEqual('internationalization .b  c d');
       expect(endSpace.substring(factory.endOffset)).toEqual('w x \'y\' z');
       expect(prefixSpace.substring(factory.prefixOffset)).toEqual('4 3 2 1');
       expect(suffixSpace.substring(0, factory.suffixOffset))
           .toEqual('1 2 3 4');

       expect(factory.embiggen()).toEqual(false);
     });

  it('does not include extraneous characters in Japanese fragments',
     function() {
       requireIntlSegmenterSupport();
       document.body.innerHTML = __html__['jp-sentence.html'];

       // Pick out 3 words from the sentence.
       const target = document.createRange();
       const selection = window.getSelection();
       const root = document.getElementById('root').firstChild;
       target.setStart(root, 25);
       target.setEnd(root, 30);
       expect(target.toString()).toEqual('てこの崇高');
       selection.removeAllRanges();
       selection.addRange(target);

       const output = generationUtils.generateFragment(selection);
       expect(output.status)
           .toEqual(generationUtils.GenerateFragmentStatus.SUCCESS);
       expect(output.fragment.textStart).toEqual('てこの崇高');
     });

  it('returns true if the first iteration is unique, even if no more' +
         'iterations are possible',
     function() {
       // It's possible that our first round of embiggening fails but the
       // initial state is nonetheless uniquely identifying. This happens if
       // the target text is very short but no context is available.
       document.body.innerHTML = __html__['first-iteration-works.html'];

       // Pick out 3 words from the sentence.
       const target = document.createRange();
       const selection = window.getSelection();
       const root = document.body.firstChild;
       target.selectNodeContents(root);
       selection.removeAllRanges();
       selection.addRange(target);

       const output = generationUtils.generateFragment(selection);
       expect(output.status)
           .toEqual(generationUtils.GenerateFragmentStatus.SUCCESS);
     });

  it('Given selection that starts and ends in non text nodes\n' +
         'When generateFragment is called\n' +
         'Then the returned fragment targets the text within the selection',
     function() {
       document.body.innerHTML = __html__['no-text-after-block-boundary.html'];

       const start = document.getElementById('1');
       const end = document.getElementById('2');
       const range = document.createRange();
       range.setStartBefore(start);
       range.setEndAfter(end);

       const selection = window.getSelection();
       selection.removeAllRanges();
       selection.addRange(range);

       const generatedFragment = generationUtils.generateFragment(selection);
       const expectedFragment = {
         status: generationUtils.GenerateFragmentStatus.SUCCESS,
         fragment: {textStart: 'text'},
       };

       expect(generatedFragment).toEqual(expectedFragment);
     });

  it('Given selection does not include visible text\n' +
         'When generateFragment is called\n' +
         'Then INVALID_SELECTION is returned',
     function() {
       document.body.innerHTML = __html__['no-text-nodes-in-range.html'];

       const start = document.getElementById('1');
       const end = document.getElementById('2');
       const range = document.createRange();
       range.setStartBefore(start);
       range.setEndAfter(end);

       const selection = window.getSelection();
       selection.removeAllRanges();
       selection.addRange(range);

       const generatedFragment = generationUtils.generateFragment(selection);
       const expectedFragment = {
         status: generationUtils.GenerateFragmentStatus.INVALID_SELECTION
       };

       expect(generatedFragment).toEqual(expectedFragment);
     });

  // BlockTextAccumulator tests
  it('Given non empty text has been appended\n' +
         'and block node has been appended\n' +
         'and another text node is appended\n' +
         'and another block node is appended\n' +
         'When textInBlock is queried\n' +
         'Then text after first block node is ignored',
     function() {
       document.body.innerHTML = __html__['block-accumulator-test.html'];

       // Create range containing the whole body to pass to the accumulator.
       const root = document.getElementById('a');
       const range = document.createRange();
       range.setStartBefore(root);
       range.setEndAfter(root);

       const accumulator =
           new generationUtils.forTesting.BlockTextAccumulator(range, true);

       // Feed non empty text node to the accumulator and then a block node to
       // indicate we reached a block boundary.
       const firstBlockNode = document.getElementById('e');
       const initialText = firstBlockNode.firstChild;
       accumulator.appendNode(initialText);
       accumulator.appendNode(firstBlockNode);

       // Feed more text and another block node to validate that text after the
       // first block node is ignored.
       const secondBlockNode = document.getElementById('f');
       accumulator.appendNode(firstBlockNode.nextSibling);
       accumulator.appendNode(secondBlockNode);

       // Verify the next sibling of the block node is a text node to make sure
       // the test fails in case we change the html test file.
       expect(firstBlockNode.nextSibling.nodeType).toEqual(Node.TEXT_NODE);

       // Check that the accumulator ignored the text after the block boundary.
       expect(accumulator.textInBlock).toEqual(initialText.textContent.trim());
     });

  it('Given non empty text has been appended\n' +
         'and block node has not been appended\n' +
         'When textInBlock is queried\n' +
         'Then textInBlock is null',
     function() {
       document.body.innerHTML = __html__['block-accumulator-test.html'];

       // Create range containing the whole body to pass to the accumulator.
       const root = document.getElementById('a');
       const range = document.createRange();
       range.setStartBefore(root);
       range.setEndAfter(root);

       const accumulator =
           new generationUtils.forTesting.BlockTextAccumulator(range, true);

       const initialText = root.firstChild;
       // Verify the first child of root is a text node to make sure the test
       // fails in case we change the html test file.
       expect(initialText.TEXT_NODE).toEqual(Node.TEXT_NODE);
       accumulator.appendNode(initialText);

       expect(accumulator.textInBlock).toBeNull();
     });

  it('Given empty text has been appended\n' +
         'and block node has been appended\n' +
         'When textInBlock is queried\n' +
         'Then textInBlock is null',
     function() {
       document.body.innerHTML = __html__['block-accumulator-test.html'];

       // Create range containing the whole body to pass to the accumulator.
       const root = document.getElementById('a');
       const range = document.createRange();
       range.setStartBefore(root);
       range.setEndAfter(root);

       const accumulator =
           new generationUtils.forTesting.BlockTextAccumulator(range, true);

       const blockNode = document.getElementById('e');
       const initialText = blockNode.previousSibling;
       // Verify initialText is a text node to make sure the test
       // fails in case we change the html test file.
       expect(initialText.TEXT_NODE).toEqual(Node.TEXT_NODE);
       // Verify initialText is empty after trimmed.
       expect(initialText.textContent.trim()).toEqual('');

       accumulator.appendNode(initialText);
       accumulator.appendNode(blockNode);

       expect(accumulator.textInBlock).toBeNull();
     });

  it('Given non empty text has been appended\n' +
         'and block node has been appended\n' +
         'When textInBlock is queried\n' +
         'Then textInBlock is the appended text after trimming',
     function() {
       document.body.innerHTML = __html__['block-accumulator-test.html'];

       // Create range containing the whole body to pass to the accumulator.
       const root = document.getElementById('a');
       const range = document.createRange();
       range.setStartBefore(root);
       range.setEndAfter(root);

       const accumulator =
           new generationUtils.forTesting.BlockTextAccumulator(range, true);

       const blockNode = document.getElementById('b');
       const initialText = blockNode.previousSibling;
       // Verify initialText is a text node to make sure the test
       // fails in case we change the html test file.
       expect(initialText.TEXT_NODE).toEqual(Node.TEXT_NODE);
       // Verify initialText is not empty after trimmed.
       expect(initialText.textContent.trim().length).toBeGreaterThan(0);

       accumulator.appendNode(initialText);
       accumulator.appendNode(blockNode);

       expect(accumulator.textInBlock).toEqual(initialText.textContent.trim());
     });

  it('Given non empty text has been appended\n' +
         'and block node has been appended\n' +
         'and range contains only a suffix of the appended text\n' +
         'When textInBlock is queried\n' +
         'Then textInBlock is the trimmed suffix of appended text that is ' +
         'contained by the range\n',
     function() {
       document.body.innerHTML = __html__['block-accumulator-test.html'];

       // Create range containing a suffix of the text passed to the
       // accumulator.
       const root = document.getElementById('e');
       const initialText = root.firstChild;
       const range = document.createRange();
       range.setStart(initialText, 2);
       range.setEndAfter(root);

       const accumulator =
           new generationUtils.forTesting.BlockTextAccumulator(range, true);

       accumulator.appendNode(initialText);
       accumulator.appendNode(root);

       expect(accumulator.textInBlock).toEqual('ancy');
     });

  it('Given non empty text has been appended\n' +
         'and block node has been appended\n' +
         'and range contains only a prefix of the appended text\n' +
         'When textInBlock is queried\n' +
         'Then textInBlock is the trimmed prefix of appended text that is ' +
         'contained by the range\n',
     function() {
       document.body.innerHTML = __html__['block-accumulator-test.html'];

       // Create range containing a suffix of the text passed to the
       // accumulator.
       const root = document.getElementById('e');
       const initialText = root.firstChild;
       const range = document.createRange();
       range.setStartBefore(initialText);
       range.setEnd(initialText, 2);

       const accumulator =
           new generationUtils.forTesting.BlockTextAccumulator(range, true);

       accumulator.appendNode(initialText);
       accumulator.appendNode(root);

       expect(accumulator.textInBlock).toEqual('f');
     });

  it('Given only non text nodes have been appended\n' +
         'and block node has been appended\n' +
         'When textInBlock is queried\n' +
         'Then textInBlock is null',
     function() {
       document.body.innerHTML = __html__['block-accumulator-test.html'];

       // Create range containing the whole body to pass to the accumulator.
       const root = document.getElementById('a');
       const range = document.createRange();
       range.setStartBefore(root);
       range.setEndAfter(root);

       const accumulator =
           new generationUtils.forTesting.BlockTextAccumulator(range, true);

       const italicNode = document.getElementById('c');
       const spanNode = document.getElementById('d');
       const blockNode = document.getElementById('b');

       // Appending non text nodes should not affect the calculated text.
       accumulator.appendNode(italicNode);
       accumulator.appendNode(spanNode);
       accumulator.appendNode(blockNode);

       expect(accumulator.textInBlock).toEqual(null);
     });

  // Test expanding ranges to word bounds when there's no text nodes in the
  // block.
  it('Given range with no text nodes in the same block as startContainer\n' +
         'When expandRangeStartToWordBound is called\n' +
         'Then range is returned without changes',
     function() {
       document.body.innerHTML = __html__['no-text-nodes-in-range.html'];

       const startNode = document.getElementById('1');
       const endNode = document.getElementById('2');

       const range = document.createRange();
       range.setStartBefore(startNode);
       range.setEndAfter(endNode);

       const expectedRange = range.cloneRange();

       generationUtils.forTesting.expandRangeStartToWordBound(range);

       expect(range).toEqual(expectedRange);
     });

  it('Given range with no text nodes in the same block as endContainer\n' +
         'When expandRangeEndToWordBound is called\n' +
         'Then range is returned without changes',
     function() {
       document.body.innerHTML = __html__['no-text-nodes-in-range.html'];

       const startNode = document.getElementById('1');
       const endNode = document.getElementById('2');

       const range = document.createRange();
       range.setStartBefore(startNode);
       range.setEndAfter(endNode);

       const expectedRange = range.cloneRange();

       generationUtils.forTesting.expandRangeEndToWordBound(range);

       expect(range).toEqual(expectedRange);
     });

  // getFirstTextNode tests.
  it('Given range with no visible text nodes\n' +
         'When getFirstNode is called\n' +
         'Then null is returned',
     function() {
       document.body.innerHTML = __html__['get-first-text-node-tests.html'];

       const startNode = document.getElementById('1');
       const endNode = document.getElementById('2');

       const range = document.createRange();
       range.setStartBefore(startNode);
       range.setEndAfter(endNode);

       const firstTextNode = generationUtils.forTesting.getFirstTextNode(range);

       expect(firstTextNode).toBeNull();
     });

  it('Given range composed of only one visible text node\n' +
         'When getFirstNode is called\n' +
         'Then the text node is returned',
     function() {
       document.body.innerHTML = __html__['get-first-text-node-tests.html'];

       const textNode = document.getElementById('3').firstChild;

       const range = document.createRange();
       range.selectNodeContents(textNode)

       const firstTextNode = generationUtils.forTesting.getFirstTextNode(range);

       expect(firstTextNode).toBe(textNode);
     });

  it('Given range with first visible text node after first node\n' +
         'When getFirstNode is called\n' +
         'Then the first visible text node is returned',
     function() {
       document.body.innerHTML = __html__['get-first-text-node-tests.html'];

       const spanNode = document.getElementById('4');

       const range = document.createRange();
       range.selectNode(spanNode)

       const firstTextNode = generationUtils.forTesting.getFirstTextNode(range);

       expect(firstTextNode).toBe(spanNode.firstChild);
     });

  it('Given range multiple visible text nodes \n' +
         'When getFirstNode is called\n' +
         'Then the first visible text node is returned',
     function() {
       document.body.innerHTML = __html__['get-first-text-node-tests.html'];

       const spanNode = document.getElementById('5');

       const range = document.createRange();
       range.selectNode(spanNode)

       const firstTextNode = generationUtils.forTesting.getFirstTextNode(range);

       expect(firstTextNode).toBe(spanNode.firstChild);
     });

  it('Given range first visible text node at its end \n' +
         'When getFirstNode is called\n' +
         'Then the first visible text node is returned',
     function() {
       document.body.innerHTML = __html__['get-first-text-node-tests.html'];

       const spanNode = document.getElementById('6');

       const range = document.createRange();
       range.selectNode(spanNode);
       range.setEnd(
           spanNode.firstChild, spanNode.firstChild.textContent.length);

       const firstTextNode = generationUtils.forTesting.getFirstTextNode(range);

       expect(firstTextNode).toBe(spanNode.firstChild);
     });

  // getLastTextNode tests.
  it('Given range with no visible text nodes\n' +
         'When getLastNode is called\n' +
         'Then null is returned',
     function() {
       document.body.innerHTML = __html__['get-last-text-node-tests.html'];

       const startNode = document.getElementById('1');
       const endNode = document.getElementById('2');

       const range = document.createRange();
       range.setStartBefore(startNode);
       range.setEndAfter(endNode);

       const lastTextNode = generationUtils.forTesting.getLastTextNode(range);

       expect(lastTextNode).toBeNull();
     });

  it('Given range composed of only one visible text node\n' +
         'When getLastNode is called\n' +
         'Then the text node is returned',
     function() {
       document.body.innerHTML = __html__['get-last-text-node-tests.html'];

       const textNode = document.getElementById('3').firstChild;

       const range = document.createRange();
       range.selectNodeContents(textNode)

       const lastTextNode = generationUtils.forTesting.getLastTextNode(range);

       expect(lastTextNode).toBe(textNode);
     });

  it('Given range with last visible text node before last node\n' +
         'When getLastNode is called\n' +
         'Then the last visible text node is returned',
     function() {
       document.body.innerHTML = __html__['get-last-text-node-tests.html'];

       const spanNode = document.getElementById('4');

       const range = document.createRange();
       range.selectNode(spanNode)

       const lastTextNode = generationUtils.forTesting.getLastTextNode(range);

       expect(lastTextNode).toBe(spanNode.firstChild);
     });

  it('Given range multiple visible text nodes \n' +
         'When getLastNode is called\n' +
         'Then the last visible text node is returned',
     function() {
       document.body.innerHTML = __html__['get-last-text-node-tests.html'];

       const spanNode = document.getElementById('5');

       const range = document.createRange();
       range.selectNode(spanNode)

       const lastTextNode = generationUtils.forTesting.getLastTextNode(range);

       const expectedTextNode = document.getElementById('6').firstChild;
       expect(lastTextNode).toBe(expectedTextNode);
     });

  it('Given range last visible text node at its start \n' +
         'When getLastNode is called\n' +
         'Then the last visible text node is returned',
     function() {
       document.body.innerHTML = __html__['get-last-text-node-tests.html'];

       const spanNode = document.getElementById('6');

       const range = document.createRange();
       range.selectNode(spanNode);
       range.setStart(spanNode.firstChild, 0);

       const lastTextNode = generationUtils.forTesting.getLastTextNode(range);

       expect(lastTextNode).toBe(spanNode.firstChild);
     });

  // moveRangeEdgesToTextNodes tests.
  it('Given a range that does not include visible text\n' +
         'When moveRangeEdgesToTextNodes is called\n' +
         'Then the range is collapsed',
     function() {
       document.body.innerHTML = __html__['no-text-nodes-in-range.html'];

       const start = document.getElementById('1');
       const end = document.getElementById('2');
       const range = document.createRange();
       range.setStartBefore(start);
       range.setEndAfter(end);

       generationUtils.forTesting.moveRangeEdgesToTextNodes(range);

       expect(range.collapsed).toBeTrue();
     });

  it('Given a range that includes visible text but not on the edges\n' +
         'When moveRangeEdgesToTextNodes is called\n' +
         'Then the range edges are at the start and end of the visible text',
     function() {
       document.body.innerHTML =
           __html__['move-range-edges-to-text-tests.html'];

       const div = document.getElementById('1');
       const range = document.createRange();
       range.selectNodeContents(div);

       generationUtils.forTesting.moveRangeEdgesToTextNodes(range);

       const textNode = document.getElementById('2').firstChild;
       expect(range.startContainer).toBe(textNode);
       expect(range.startOffset).toBe(0);
       expect(range.endContainer).toBe(textNode);
       expect(range.endOffset).toBe(textNode.textContent.length);
     });

  it('Given a range that includes visible text on the edges\n' +
         'When moveRangeEdgesToTextNodes is called\n' +
         'Then the range edges are not changed',
     function() {
       document.body.innerHTML =
           __html__['move-range-edges-to-text-tests.html'];

       const span = document.getElementById('3');
       const range = document.createRange();
       range.selectNodeContents(span);

       const originalRange = range.cloneRange();

       generationUtils.forTesting.moveRangeEdgesToTextNodes(range);

       expect(range.startContainer).toBe(originalRange.startContainer);
       expect(range.startOffset).toBe(originalRange.startOffset);
       expect(range.endContainer).toBe(originalRange.endContainer);
       expect(range.endOffset).toBe(originalRange.endOffset);
     });
});
