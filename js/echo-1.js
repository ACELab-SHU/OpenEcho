(() => {
  'use strict';
  const stages = [
    ['Describe the computation', 'Express Venus tasks in C and their dependencies in BAS. Keep algorithms, inputs and interfaces explicit.', 'Venus C + BAS → tasks + dependencies'],
    ['Build a reproducible DAG', 'Compile task images and the DAG description with the Venus-custom toolchain. Keep the generated artifacts together.', './ace-echo compile dag --target nrPBCH'],
    ['Inspect execution in Gem5', 'Run the application contract in fast mode and inspect task execution, data movement and cycle-level timing.', './ace-echo run application --l1-elf l1.elf --mode fast'],
    ['Compare with an RTL reference', 'Compare matching task boundaries and returned data. Keep unknown bits, independent golden checks and timing scope visible.', 'Task timing + output comparison → evidence']
  ];
  const tabs = [...document.querySelectorAll('[data-stage]')];
  const panel = document.querySelector('#stage-panel');
  function selectStage(index, focus = false) {
    tabs.forEach((button, i) => { button.setAttribute('aria-selected', String(i === index)); button.tabIndex = i === index ? 0 : -1; });
    const [title, body, command] = stages[index];
    panel.querySelector('h3').textContent = title;
    panel.querySelector('p').textContent = body;
    panel.querySelector('code').textContent = command;
    panel.setAttribute('aria-labelledby', tabs[index].id);
    document.querySelectorAll('[data-node]').forEach(node => node.classList.toggle('node-active', Number(node.dataset.node) === index));
    if (focus) tabs[index].focus();
  }
  tabs.forEach((button, index) => {
    button.addEventListener('click', () => selectStage(index));
    button.addEventListener('keydown', event => {
      const offsets = { ArrowRight: 1, ArrowLeft: -1 };
      if (event.key in offsets) { event.preventDefault(); selectStage((index + offsets[event.key] + tabs.length) % tabs.length, true); }
      else if (event.key === 'Home' || event.key === 'End') { event.preventDefault(); selectStage(event.key === 'Home' ? 0 : tabs.length - 1, true); }
    });
  });
  document.querySelectorAll('[data-filter]').forEach(button => button.addEventListener('click', () => {
    const filter = button.dataset.filter;
    document.querySelectorAll('[data-filter]').forEach(b => b.setAttribute('aria-pressed', String(b === button)));
    let count = 0;
    document.querySelectorAll('.results tbody tr').forEach(row => { row.hidden = filter !== 'all' && row.dataset.family !== filter; if (!row.hidden) count++; });
    document.querySelector('#result-count').textContent = `${count} DAGs · Venus 1.0 · fast mode`;
  }));
  document.querySelector('#copy-command').addEventListener('click', async () => {
    const status = document.querySelector('#copy-status');
    try { await navigator.clipboard.writeText(document.querySelector('#clone-command').textContent); status.textContent = 'Clone command copied.'; }
    catch { status.textContent = 'Select and copy the command above.'; }
  });
})();
