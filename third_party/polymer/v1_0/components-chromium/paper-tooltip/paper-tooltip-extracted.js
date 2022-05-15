Polymer({
      is: 'paper-tooltip',
      hostAttributes: {
        role: 'tooltip',
        tabindex: -1
      },
      properties: {
        /**
         * The id of the element that the tooltip is anchored to. This element
         * must be a sibling of the tooltip. If this property is not set,
         * then the tooltip will be centered to the parent node containing it.
         */
        for: {
          type: String,
          observer: '_findTarget'
        },
        /**
         * Set this to true if you want to manually control when the tooltip
         * is shown or hidden.
         */
        manualMode: {
          type: Boolean,
          value: false,
          observer: '_manualModeChanged'
        },
        /**
         * Positions the tooltip to the top, right, bottom, left of its content.
         */
        position: {
          type: String,
          value: 'bottom'
        },
        /**
         * If true, no parts of the tooltip will ever be shown offscreen.
         */
        fitToVisibleBounds: {
          type: Boolean,
          value: false
        },
        /**
         * The spacing between the top of the tooltip and the element it is
         * anchored to.
         */
        offset: {
          type: Number,
          value: 14
        },
        /**
         * This property is deprecated, but left over so that it doesn't
         * break exiting code. Please use `offset` instead. If both `offset` and
         * `marginTop` are provided, `marginTop` will be ignored.
         * @deprecated since version 1.0.3
         */
        marginTop: {
          type: Number,
          value: 14
        },
        /**
         * The delay that will be applied before the `entry` animation is
         * played when showing the tooltip.
         */
        animationDelay: {
          type: Number,
          value: 500,
          observer: '_delayChange'
        },
        /**
         * The animation that will be played on entry.  This replaces the
         * deprecated animationConfig.  Entries here will override the 
         * animationConfig settings.  You can enter your own animation
         * by setting it to the css class name.
         */
        animationEntry: {
          type: String,
          value: ""
        },
        /**
         * The animation that will be played on exit.  This replaces the
         * deprecated animationConfig.  Entries here will override the 
         * animationConfig settings.  You can enter your own animation
         * by setting it to the css class name.
         */
        animationExit: {
          type: String,
          value: ""
        },
        /**
         * This property is deprecated.  Use --paper-tooltip-animation to change the animation.
         * The entry and exit animations that will be played when showing and
         * hiding the tooltip. If you want to override this, you must ensure
         * that your animationConfig has the exact format below.
         * @deprecated since version
         *
         * The entry and exit animations that will be played when showing and
         * hiding the tooltip. If you want to override this, you must ensure
         * that your animationConfig has the exact format below.
         */
        animationConfig: {
          type: Object,
          value: function () {
            return {
              'entry': [{
                name: 'fade-in-animation',
                node: this,
                timing: {
                  delay: 0
                }
              }],
              'exit': [{
                name: 'fade-out-animation',
                node: this
              }]
            }
          }
        },
        _showing: {
          type: Boolean,
          value: false
        }
      },
      listeners: {
        'webkitAnimationEnd': '_onAnimationEnd',
      },
      /**
       * Returns the target element that this tooltip is anchored to. It is
       * either the element given by the `for` attribute, the element manually
       * specified through the `target` attribute, or the immediate parent of
       * the tooltip.
       *
       * @type {Node}
       */
      get target() {
        if (this._manualTarget)
          return this._manualTarget;

        var parentNode = Polymer.dom(this).parentNode;
        // If the parentNode is a document fragment, then we need to use the host.
        var ownerRoot = Polymer.dom(this).getOwnerRoot();
        var target;
        if (this.for) {
          target = Polymer.dom(ownerRoot).querySelector('#' + this.for);
        } else {
          target = parentNode.nodeType == Node.DOCUMENT_FRAGMENT_NODE ?
              ownerRoot.host : parentNode;
        }
        return target;
      },

      /**
       * Sets the target element that this tooltip will be anchored to.
       * @param {Node} target
       */
      set target(target) {
        this._manualTarget = target;
        this._findTarget();
      },

      /**
       * @return {void}
       */
      attached: function () {
        this._findTarget();
      },

      /**
       * @return {void}
       */
      detached: function () {
        if (!this.manualMode)
          this._removeListeners();
      },

      /**
       * Replaces Neon-Animation playAnimation - just calls show and hide.
       * @deprecated Use show and hide instead.
       * @param {string} type Either `entry` or `exit`
       */
      playAnimation: function (type) {
        if (type === "entry") {
          this.show();
        } else if (type === "exit") {
          this.hide();
        }
      },

      /**
       * Cancels the animation and either fully shows or fully hides tooltip
       */
      cancelAnimation: function () {
        // Short-cut and cancel all animations and hide
        this.$.tooltip.classList.add('cancel-animation');
      },

      /**
       * Shows the tooltip programatically
       * @return {void}
       */
      show: function () {
        // If the tooltip is already showing, there's nothing to do.
        if (this._showing)
          return;

        if (Polymer.dom(this).textContent.trim() === '') {
          // Check if effective children are also empty
          var allChildrenEmpty = true;
          var effectiveChildren = Polymer.dom(this).getEffectiveChildNodes();
          for (var i = 0; i < effectiveChildren.length; i++) {
            if (effectiveChildren[i].textContent.trim() !== '') {
              allChildrenEmpty = false;
              break;
            }
          }
          if (allChildrenEmpty) {
            return;
          }
        }

        this._showing = true;
        this.$.tooltip.classList.remove('hidden');
        this.$.tooltip.classList.remove('cancel-animation');
        this.$.tooltip.classList.remove(this._getAnimationType('exit'));
        this.updatePosition();
        this._animationPlaying = true;
        this.$.tooltip.classList.add(this._getAnimationType('entry'));
      },

      /**
       * Hides the tooltip programatically
       * @return {void}
       */
      hide: function () {
        // If the tooltip is already hidden, there's nothing to do.
        if (!this._showing) {
          return;
        }

        // If the entry animation is still playing, don't try to play the exit
        // animation since this will reset the opacity to 1. Just end the animation.
        if (this._animationPlaying) {
          this._showing = false;
          this._cancelAnimation();
          return;
        } else {
          // Play Exit Animation
          this._onAnimationFinish();
        }

        this._showing = false;
        this._animationPlaying = true;
      },

      /**
       * @return {void}
       */
      updatePosition: function () {
        if (!this._target || !this.offsetParent)
          return;
        var offset = this.offset;
        // If a marginTop has been provided by the user (pre 1.0.3), use it.
        if (this.marginTop != 14 && this.offset == 14)
          offset = this.marginTop;
        var parentRect = this.offsetParent.getBoundingClientRect();
        var targetRect = this._target.getBoundingClientRect();
        var thisRect = this.getBoundingClientRect();
        var horizontalCenterOffset = (targetRect.width - thisRect.width) / 2;
        var verticalCenterOffset = (targetRect.height - thisRect.height) / 2;
        var targetLeft = targetRect.left - parentRect.left;
        var targetTop = targetRect.top - parentRect.top;
        var tooltipLeft, tooltipTop;
        switch (this.position) {
          case 'top':
            tooltipLeft = targetLeft + horizontalCenterOffset;
            tooltipTop = targetTop - thisRect.height - offset;
            break;
          case 'bottom':
            tooltipLeft = targetLeft + horizontalCenterOffset;
            tooltipTop = targetTop + targetRect.height + offset;
            break;
          case 'left':
            tooltipLeft = targetLeft - thisRect.width - offset;
            tooltipTop = targetTop + verticalCenterOffset;
            break;
          case 'right':
            tooltipLeft = targetLeft + targetRect.width + offset;
            tooltipTop = targetTop + verticalCenterOffset;
            break;
        }
        // TODO(noms): This should use IronFitBehavior if possible.
        if (this.fitToVisibleBounds) {
          // Clip the left/right side
          if (parentRect.left + tooltipLeft + thisRect.width > window.innerWidth) {
            this.style.right = '0px';
            this.style.left = 'auto';
          } else {
            this.style.left = Math.max(0, tooltipLeft) + 'px';
            this.style.right = 'auto';
          }
          // Clip the top/bottom side.
          if (parentRect.top + tooltipTop + thisRect.height > window.innerHeight) {
            this.style.bottom = parentRect.height + 'px';
            this.style.top = 'auto';
          } else {
            this.style.top = Math.max(-parentRect.top, tooltipTop) + 'px';
            this.style.bottom = 'auto';
          }
        } else {
          this.style.left = tooltipLeft + 'px';
          this.style.top = tooltipTop + 'px';
        }
      },
      _addListeners: function () {
        if (this._target) {
          this.listen(this._target, 'mouseenter', 'show');
          this.listen(this._target, 'focus', 'show');
          this.listen(this._target, 'mouseleave', 'hide');
          this.listen(this._target, 'blur', 'hide');
          this.listen(this._target, 'tap', 'hide');
        }
        this.listen(this.$.tooltip, 'animationend', '_onAnimationEnd');
        this.listen(this, 'mouseenter', 'hide');
      },
      _findTarget: function () {
        if (!this.manualMode)
          this._removeListeners();
        this._target = this.target;
        if (!this.manualMode)
          this._addListeners();
      },
      _delayChange: function (newValue) {
        // Only Update delay if different value set
        if (newValue !== 500) {
          this.updateStyles({
            '--paper-tooltip-delay-in': newValue + "ms"
          });
        }
      },
      _manualModeChanged: function () {
        if (this.manualMode)
          this._removeListeners();
        else
          this._addListeners();
      },
      _cancelAnimation: function () {
        // Short-cut and cancel all animations and hide
        this.$.tooltip.classList.remove(this._getAnimationType('entry'));
        this.$.tooltip.classList.remove(this._getAnimationType('exit'));
        this.$.tooltip.classList.remove('cancel-animation');
        this.$.tooltip.classList.add('hidden');
      },
      _onAnimationFinish: function () {
        if (this._showing) {
          this.$.tooltip.classList.remove(this._getAnimationType('entry'));
          this.$.tooltip.classList.remove('cancel-animation');
          this.$.tooltip.classList.add(this._getAnimationType('exit'));
        }
      },
      _onAnimationEnd: function () {
        // If no longer showing add class hidden to completely hide tooltip
        this._animationPlaying = false;
        if (!this._showing) {
          this.$.tooltip.classList.remove(this._getAnimationType('exit'));
          this.$.tooltip.classList.add('hidden');
        }
      },
      _getAnimationType: function (type) {
        // These properties have priority over animationConfig values
        if ((type === 'entry') && (this.animationEntry !== '')) {
          return this.animationEntry;
        }
        if ((type === 'exit') && (this.animationExit !== '')) {
          return this.animationExit;
        }
        // If no results then return the legacy value from animationConfig
        if (this.animationConfig[type] && typeof this.animationConfig[type][0].name === 'string') {
          // Checking Timing and Update if necessary - Legacy for animationConfig
          if (this.animationConfig[type][0].timing &&
            this.animationConfig[type][0].timing.delay &&
            this.animationConfig[type][0].timing.delay !== 0) {
            var timingDelay = this.animationConfig[type][0].timing.delay;
            // Has Timing Change - Update CSS
            if (type === 'entry') {
              this.updateStyles({
                '--paper-tooltip-delay-in': timingDelay + "ms"
              });
            } else if (type === 'exit') {
              this.updateStyles({
                '--paper-tooltip-delay-out': timingDelay + "ms"
              });
            }
          }
          return this.animationConfig[type][0].name;
        }
      },
      _removeListeners: function () {
        if (this._target) {
          this.unlisten(this._target, 'mouseenter', 'show');
          this.unlisten(this._target, 'focus', 'show');
          this.unlisten(this._target, 'mouseleave', 'hide');
          this.unlisten(this._target, 'blur', 'hide');
          this.unlisten(this._target, 'tap', 'hide');
        }
        this.unlisten(this.$.tooltip, 'animationend', '_onAnimationEnd');
        this.unlisten(this, 'mouseenter', 'hide');
      }
    });